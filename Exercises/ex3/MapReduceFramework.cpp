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
const uint64_t thirty_three_64 = 33;
const uint64_t sixty_two_64 = 62;
const uint64_t phase_shift= one_64 << sixty_two_64;
const int mutex_num = 6;

/**
 * a struct of all relevant mutexes
 */
typedef struct{
    pthread_mutex_t map_mutex;
    pthread_mutex_t emit2_mutex;
    pthread_mutex_t shuffle_mutex;
    pthread_mutex_t reduce_mutex;
    pthread_mutex_t emit3_mutex;
    pthread_mutex_t cycle_mutex;
}mutex_struct;

/**
 * a struct containing all the context relevant to some job
 */
typedef struct JobContext{
    std::map<pthread_t,IntermediateVec*>& id_to_vec_map; // maps thread ids to intermediate vectors
    std::map<K2*,IntermediateVec>& key_to_vec_map; // maps k2 To vector of v2s (for shuffling)
    const MapReduceClient &client;
    JobState j_state;
    const InputVec& input_vec;
    OutputVec& out_vec;
    Barrier *barrier;
    pthread_t zero_thread;
    int num_threads;
    bool already_waited;
    pthread_t *threads;
    std::atomic<uint64_t> ac;
    mutex_struct mutexes;
}JobContext;


/**
 * this is actually a setter for a new state
 * @param job
 * @param state - new state to assign
 */
void getJobState(JobHandle job, JobState* state) {
    auto jc = (JobContext *) job;
    if (jc->j_state.stage == MAP_STAGE) {
        uint64_t z = pow(2, 31) - 1;
        unsigned long processed = jc->ac & z;
        jc->j_state.percentage = 100 * (float) processed / (float) jc->input_vec.size();
    } else if (jc->j_state.stage == SHUFFLE_STAGE) {
        int processed = 0;
        for (const auto &vecMap : jc->key_to_vec_map) {
            processed += vecMap.second.size();
        }
        int size = (jc->ac << two_64) >> thirty_three_64;
        jc->j_state.percentage = 100 * (float) processed / (float) size;
    } else if (jc->j_state.stage == REDUCE_STAGE) {
        int size = (jc->ac << two_64) >> thirty_three_64;
        jc->j_state.percentage = 100 * (float) size / (float) jc->key_to_vec_map.size();
    }
    jc->j_state.stage = state->stage;
}

/**
 * locks a given mutex
 * @param mutex
 */
void lock_mutex(pthread_mutex_t *mutex){
    int ret = pthread_mutex_lock(mutex); // TODO: remove this when fixed couldn't lock mutex problem (ret = 22)
    if (ret != 0){
        std::cerr << MUTEX_LOCK_ERR << ret << std::endl;
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
std::vector<IntermediateVec> convert_map_type(const std::map<K2*,IntermediateVec>& key_to_vec_map){
    std::vector<IntermediateVec> ret;
    for (auto &elem: key_to_vec_map){
        IntermediateVec v;
        for (auto &pair: elem.second){
            v.push_back(pair);
        }
        ret.push_back(v);
    }
    return ret;
}

/**
 * performs the map phase
 * @param jc
 */
void map_phase(JobContext *jc) {
    uint64_t old_value= 0;
    uint64_t z= pow(2, 31) - 1;
    int input_size = jc->input_vec.size();
    jc->j_state = {MAP_STAGE, 0};
    jc->ac += phase_shift; // TODO: changed from 00 to 10 instead of 01
    while ((old_value & z) < input_size-1){
        lock_mutex(&jc->mutexes.map_mutex);
        old_value = ((jc->ac))++;
        if((old_value & z) < input_size){
            InputPair pair = jc->input_vec[(int) (old_value & z)];
            jc->client.map(pair.first,pair.second,jc);
            unlock_mutex(&jc->mutexes.map_mutex);
            continue;
        }
        unlock_mutex(&jc->mutexes.map_mutex);
        break;
    }
}

/**
 * performs the sort phase
 * @param jc
 */
void sort_phase(JobContext *jc) {
    IntermediateVec *curr_vec = jc->id_to_vec_map[pthread_self()];
}

/**
 * performs the shuffle phase
 * @param jc
 */
void shuffle_phase(JobContext *jc) {
    int input_size = jc->input_vec.size();
    jc->j_state = {SHUFFLE_STAGE, 0};
    jc->ac += phase_shift; // TODO: doesnt really work
    jc->ac -= input_size;

    if (pthread_self()==jc->zero_thread){ // only the 0th thread shuffles.
        lock_mutex(&jc->mutexes.shuffle_mutex);
        for(auto &tid: jc->id_to_vec_map){
            IntermediateVec *cur_vec = tid.second;
            while(!cur_vec->empty()){
                IntermediatePair cur_pair = cur_vec->back();
                cur_vec->pop_back();
                bool found_item = false;
                if (jc->key_to_vec_map.empty()){
                    jc->key_to_vec_map[cur_pair.first].push_back(cur_pair);
                    (jc->ac)++;  // count number of vectors in queue
                    continue;
                }
                else{
                    for(auto &elem: jc->key_to_vec_map){
                        if(!((*elem.first < *cur_pair.first)||(*cur_pair.first < *elem.first))){
                            // cur pair's key already in the map
                            jc->key_to_vec_map[elem.first].push_back(cur_pair);
                            found_item = true;
                        }
                    }
                    if(!found_item){
                        auto * new_vec = new IntermediateVec();
                        new_vec->push_back(cur_pair);
                        (jc->ac)++;  // count number of vectors in queue
                        jc->key_to_vec_map[cur_pair.first] = *new_vec;
                    }
                }
            }
        }
        unlock_mutex(&jc->mutexes.shuffle_mutex);
    }
}

/**
 * performs the reduce phase
 * @param jc
 */
void reduce_phase(JobContext *jc) {
    jc->j_state = {REDUCE_STAGE, 0};
    jc->ac += phase_shift;

    uint64_t size = ((jc->ac << 2) >> 33) << 31; // setting the middle section to zero
    jc->ac -= size;

    uint64_t z= pow(2, 31) - 1;
    std::vector<IntermediateVec> queue = convert_map_type(jc->key_to_vec_map);
    // init old_val and the atomic counter //
    uint64_t old_value=-1;
    jc->ac --;
    while((old_value&z) > 0){
        lock_mutex(&jc->mutexes.reduce_mutex);
        old_value = (jc->ac)--;
        if((old_value&z) > 0)
        {
            const IntermediateVec cur_vec = queue[old_value&z];
        }
        //TODO:HELP MEEEE check cases to avoid SIGSEV
        const IntermediateVec cur_vec = queue[old_value&z];
        jc->client.reduce(&cur_vec,jc);
        unlock_mutex(&jc->mutexes.reduce_mutex);
    }
}


/**
 * performs an entire thread life cycle
 * @param arg the context (casted to job handler)
 * @return
 */
void* thread_cycle(void *arg){
    JobContext *jc = (JobContext*) arg;

    lock_mutex(&jc->mutexes.cycle_mutex);
    std::vector<IntermediatePair> *cur_vec = new std::vector<IntermediatePair>();
    if(jc->id_to_vec_map.empty()){
        jc->zero_thread = pthread_self();
    }
    jc->id_to_vec_map.insert({pthread_self(),cur_vec});
    unlock_mutex(&jc->mutexes.cycle_mutex);

    map_phase(jc);

    sort_phase(jc);

    jc->barrier->barrier();

    shuffle_phase(jc);

    jc->barrier->barrier();

    reduce_phase(jc);

    return nullptr;
}



JobHandle
startMapReduceJob(const MapReduceClient &client, const InputVec &inputVec, OutputVec &outputVec, int multiThreadLevel) {
    pthread_mutex_t *mutexes = (pthread_mutex_t*) malloc(mutex_num * sizeof(pthread_mutex_t));
    pthread_t *threads = (pthread_t*) malloc(multiThreadLevel * sizeof(pthread_t));
    auto id_to_vec_map = new std::map<pthread_t,IntermediateVec*>;
    auto key_to_vec_map = new std::map<K2*,IntermediateVec>;
    for(int i=0; i< mutex_num; i++){
        mutexes[i] = PTHREAD_MUTEX_INITIALIZER;
    }
    JobContext *job_c = new JobContext {
            .id_to_vec_map = *id_to_vec_map,
            .key_to_vec_map = *key_to_vec_map,
            .client = client,
            .j_state = {UNDEFINED_STAGE,0},
            .input_vec = inputVec,
            .out_vec = outputVec,
            .barrier = new Barrier(multiThreadLevel),
            .num_threads = multiThreadLevel,
            .already_waited = false,
            .threads = threads,
            .ac{0},
            .mutexes = {
                    .map_mutex = mutexes[0],
                    .emit2_mutex = mutexes[1],
                    .shuffle_mutex =mutexes[2],
                    .reduce_mutex = mutexes[3],
                    .emit3_mutex = mutexes[4],
                    .cycle_mutex = mutexes[5]
            }
    };

    pthread_mutex_t init_mutex = PTHREAD_MUTEX_INITIALIZER;
    lock_mutex(&init_mutex);
    for (int i = 0; i < multiThreadLevel; ++i){
        if(pthread_create(&threads[i], nullptr, thread_cycle, job_c)!=0){
            std::cerr << THREAD_INIT_ERR << std::endl;
            exit(EXIT_FAILURE);
        }
//        if (i==0){
//            job_c->zero_thread = pthread_self(); // distinguishing the first thread, which will do SHUFFLE
//        }
    }
    unlock_mutex(&init_mutex);
    pthread_mutex_destroy(&init_mutex);
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
    JobContext* jc = (JobContext*) context;
    pthread_t tid = pthread_self();
    lock_mutex(&jc->mutexes.emit2_mutex);
    if (jc->id_to_vec_map.find(tid)==jc->id_to_vec_map.end()){ // no vector corresponds to self thread id
        IntermediateVec *inter_vec;
        jc->id_to_vec_map.insert({tid, inter_vec});
    }
    jc->id_to_vec_map[tid]->push_back(IntermediatePair(key,value));  // adding the pair to the needed vector
    unlock_mutex(&jc->mutexes.emit2_mutex);
    uint64_t inc = 1 << 31;
    jc->ac += inc;

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
    jc->out_vec.push_back(OutputPair(key,value));
    uint64_t inc = one_64 << 31; // TODO: should be the size of the vector
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