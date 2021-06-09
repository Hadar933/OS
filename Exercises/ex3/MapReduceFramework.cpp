//
// Created by shulik10 on 06/06/2021.
//
#include "MapReduceFramework.h"
#include <vector>
#include <map>
#include <algorithm>
#include "Barrier/Barrier.h"
#include <pthread.h>
#include <iostream>
#include "math.h"

#define THREAD_INIT_ERR "system error: couldn't initialize thread"
#define MUTEX_LOCK_ERR "system error: couldn't lock mutex"
#define MUTEX_UNLOCK_ERR "system error: couldn't unlock mutex"

const uint64_t one_64 =1;
const uint64_t two_64 = 2;
const uint64_t large_r_shift = 33;

/**
 * a struct of all relevant mutexes
 */
typedef struct{
    pthread_mutex_t map_mutex;
    pthread_mutex_t emit2_mutex;
    pthread_mutex_t shuffle_mutex;
    pthread_mutex_t reduce_mutex;
    pthread_mutex_t emit3_mutex;
}mutex_struct;

/**
 * a struct containing all the context relevant to some job
 */
typedef struct JobContext{
    std::map<pthread_t,IntermediateVec> id_to_vec_map; // maps thread ids to intermediate vectors
    std::map<K2*,std::vector<V2*>> key_to_vec_map; // maps k2 To vector of v2s (for shuffling)
    const MapReduceClient *client;
    JobState j_state;
    const InputVec* input_vec;
    OutputVec* out_vec;
    Barrier *barrier;
    pthread_t zero_thread;
    int num_threads;
    bool already_waited;
    pthread_t *threads;
    std::atomic<uint64_t> ac;
//    std::atomic<uint64_t> inter_vec_atomic_count;
//    std::atomic<uint64_t> inter_vec_vec_atomic_count;
//    std::atomic<uint64_t> out_vec_atomic_count;
    mutex_struct mutexes;
}JobContext;


/**
 * this is actually a setter for a new state
 * @param job
 * @param state - new state to assign
 */
void getJobState(JobHandle job, JobState* state) {
    //TODO: infinite loop. diff threads are in diff stages which is impossible.
    auto jc = (JobContext *) job;
    if (jc->j_state.stage == MAP_STAGE) {
        uint64_t z = pow(2, 31) - 1;
        unsigned long processed = jc->ac & z;
        jc->j_state.percentage = 100 * (float) processed / (float) jc->input_vec->size();
    } else if (jc->j_state.stage == SHUFFLE_STAGE) {
        int processed = 0;
        for (const auto &vecMap : jc->key_to_vec_map) {
            processed += vecMap.second.size();
        }
        int size = (jc->ac << two_64) >> large_r_shift;
        jc->j_state.percentage = 100 * (float) processed / (float) size;
    } else if (jc->j_state.stage == REDUCE_STAGE) {
        int size = (jc->ac << two_64) >> large_r_shift;
        jc->j_state.percentage = 100 * (float) size / (float) jc->key_to_vec_map.size();
        jc->j_state.stage = state->stage;
    }
}

/**
 * locks a given mutex
 * @param mutex
 */
void lock_mutex(pthread_mutex_t *mutex){
    if (pthread_mutex_lock(mutex) != 0){
        std::cerr << MUTEX_LOCK_ERR << std::endl;
        exit(EXIT_FAILURE);
    }
}

/**
 * unlocks a given mutex
 * @param mutex
 */
void unlock_mutex(pthread_mutex_t *mutex){
    /*
     */
    if (pthread_mutex_unlock(mutex) != 0){
        std::cerr << MUTEX_UNLOCK_ERR << std::endl;
        exit(EXIT_FAILURE);
    }
}


/**
 * converts a map of k2 keys->intermediate vec to vector of intermediate vectors
 * @param key_to_vec_map
 * @return vector of intermediate vectors
 */
std::vector<IntermediateVec> convert_map_type(const std::map<K2*,std::vector<V2*>>& key_to_vec_map){
    std::vector<IntermediateVec> ret;
    for (auto &elem: key_to_vec_map){
        IntermediateVec v;
        for (auto &item: elem.second){
            v.push_back(IntermediatePair(elem.first,item));
        }
        ret.push_back(v);
    }
    return ret;
}

/**
 * performs an entire thread life cycle
 * @param arg the context (casted to job handler)
 * @return
 */
void* thread_cycle(void *arg){

    // MAP PHASE //
    auto *jc = (JobContext*) arg;
    JobState stage = {MAP_STAGE, 0};
    uint64_t phase_shift = one_64 << 62;
    jc->ac += phase_shift;
    getJobState(jc,&stage);
    int input_size = jc->input_vec->size();
    uint64_t old_value = 0;
    uint64_t z  = pow(2,31) - 1;
    while ((old_value & z)  < input_size){
        lock_mutex(&jc->mutexes.map_mutex);
        old_value = ((jc->ac))++;
        InputPair pair = jc->input_vec->at(old_value & z);
        jc->client->map(pair.first,pair.second,arg);
        unlock_mutex(&jc->mutexes.map_mutex);
    }

    // SORT PHASE //
    IntermediateVec curr_vec = jc->id_to_vec_map[pthread_self()];
    std::sort(curr_vec.begin(),curr_vec.end(), [](IntermediatePair p1, IntermediatePair p2){
        return *p2.first < *p1.first;
    }); // sorting according to K2 (first)
    jc->barrier->barrier();

    // SHUFFLE PHASE //
    jc->j_state = {SHUFFLE_STAGE, 0}; jc->ac += phase_shift;  jc->ac -= input_size;

    if (pthread_self()==jc->zero_thread){ // only the 0th thread shuffles.
        lock_mutex(&jc->mutexes.shuffle_mutex);
        for(auto &tid: jc->id_to_vec_map){
            IntermediateVec cur_vec = tid.second;
            while(!cur_vec.empty()){
                IntermediatePair cur_pair = cur_vec.back();
                cur_vec.pop_back();
                if (jc->key_to_vec_map.find(cur_pair.first)==jc->key_to_vec_map.end()){ // no vector with such key
                    std::vector<V2*> v;
                    jc->key_to_vec_map[cur_pair.first] = v;
                    (jc->ac)++;  // count number of vectors in queue
                }
                jc->key_to_vec_map[cur_pair.first].push_back(cur_pair.second);  // adding the pair to the needed vector
            }
        }
        unlock_mutex(&jc->mutexes.shuffle_mutex);
    }
    jc->barrier->barrier();

    // REDUCE PHASE //
    jc->j_state = {REDUCE_STAGE, 0};jc->ac += phase_shift;

    std::vector<IntermediateVec> queue = convert_map_type(jc->key_to_vec_map);
    // init old_val and the atomic counter //
    old_value=0;
    while((old_value&z) >= 0){
        old_value = (jc->ac)--;
        const IntermediateVec cur_vec = queue[old_value&z];
        jc->client->reduce(&cur_vec,jc);
    }
    return nullptr;
}

JobHandle
startMapReduceJob(const MapReduceClient &client, const InputVec &inputVec, OutputVec &outputVec, int multiThreadLevel) {
    auto *threads = new pthread_t(multiThreadLevel);

    JobContext job_c = { .id_to_vec_map = {},
            .client = &client,
            .j_state = {UNDEFINED_STAGE,0},
            .input_vec = &inputVec,
            .out_vec = &outputVec,
            .barrier = new Barrier(multiThreadLevel),
            .num_threads = multiThreadLevel,
            .already_waited = false,
            .threads = threads,
            .ac{0},
//            .inter_vec_atomic_count{0},
//            .inter_vec_vec_atomic_count{0},
//            .out_vec_atomic_count{0},
            .mutexes = {PTHREAD_MUTEX_INITIALIZER,PTHREAD_MUTEX_INITIALIZER,
                        PTHREAD_MUTEX_INITIALIZER,PTHREAD_MUTEX_INITIALIZER,
                        PTHREAD_MUTEX_INITIALIZER},

    };
    for (int i = 0; i < multiThreadLevel; ++i){
        if(pthread_create(threads + i, nullptr, thread_cycle, &job_c)!=0){
            std::cerr << THREAD_INIT_ERR << std::endl;
            exit(EXIT_FAILURE);
        } // TODO handle success or fail return values
        if (i==0){
            job_c.zero_thread = pthread_self(); // distinguishing the first thread, which will do SHUFFLE
        }
    }
    auto jc = static_cast<JobHandle>(&job_c);
    return jc;
}

/**
 * saves the intermediary element in a dictionary of tid: intermediary vector
 * and updates the number of intermediary elements using atomic counter
 * @param key
 * @param value
 * @param context
 */
void emit2 (K2* key, V2* value, void* context){
    auto jc = (JobContext*) context;
    lock_mutex(&jc->mutexes.emit2_mutex);
    pthread_t tid = pthread_self();
    if (jc->id_to_vec_map.find(tid)==jc->id_to_vec_map.end()){ // no vector corresponds to self thread id
        IntermediateVec inter_vec;
        jc->id_to_vec_map[tid] = inter_vec;
    }
    jc->id_to_vec_map[tid].push_back(IntermediatePair(key,value));  // adding the pair to the needed vector
    uint64_t inc = 1 << 31;
    jc->ac += inc;
    unlock_mutex(&jc->mutexes.emit2_mutex);
}

/**
 * saves the output element in outputVec (inside context) and updates the number of elements
 * using atomic counter
 * @param key
 * @param value
 * @param context
 */
void emit3 (K3* key, V3* value, void* context){
    auto jc = (JobContext*) context;
    lock_mutex(&jc->mutexes.emit3_mutex);
    jc->out_vec->push_back(OutputPair(key,value));
    uint64_t inc = 1 << 31;
    jc->ac += inc;
    unlock_mutex(&jc->mutexes.emit3_mutex);
}

/**
 * uses pthread_join to wait for jobs.
 * @param job
 */
void waitForJob(JobHandle job){
    auto jc = (JobContext*) job;
    if (!jc->already_waited){
        for (int i = 0; i < jc->num_threads; ++i) {
            pthread_join(jc->threads[i], nullptr);
        }
        jc->already_waited = true;
    }
}

/**
 * destroys all the mutexes
 * @param jc
 */
void destroy_all_mutex(JobContext* jc){
    pthread_mutex_destroy(&jc->mutexes.map_mutex);
    pthread_mutex_destroy(&jc->mutexes.emit2_mutex);
    pthread_mutex_destroy(&jc->mutexes.shuffle_mutex);
    pthread_mutex_destroy(&jc->mutexes.reduce_mutex);
    pthread_mutex_destroy(&jc->mutexes.emit3_mutex);
}

/**
 * closes a job - destroys mutex and frees memory
 * @param job
 */
void closeJobHandle(JobHandle job){
    auto jc = (JobContext*) job;
    waitForJob(job);
    destroy_all_mutex(jc);
    delete jc->barrier;
}