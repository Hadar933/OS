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
//typedef std::vector<> vector;


typedef struct{
    pthread_mutex_t map_mutex;
    pthread_mutex_t emit2_mutex;
    pthread_mutex_t shuffle_mutex;
    pthread_mutex_t reduce_mutex;
    pthread_mutex_t emit3_mutex;
}mutex_struct;

typedef struct{
    std::map<pthread_t,IntermediateVec> id_to_vec_map; // maps thread ids to intermediate vectors
    std::map<K2*,std::vector<V2*>> key_to_vec_map; // maps k2 To vector of v2s (for shuffling)
    const MapReduceClient *client;
    JobState j_state;
    const InputVec input_vec;
    OutputVec out_vec;
    Barrier *barrier;
    pthread_t zero_thread;
    int num_threads;
    bool already_waited;
    pthread_t *threads;
    std::atomic<uint32_t>* jobs_atomic_count;
    std::atomic<uint32_t>* inter_vec_atomic_count;
    std::atomic<uint32_t>* inter_vec_vec_atomic_count;
    std::atomic<uint32_t>* out_vec_atomic_count;
    mutex_struct mutexes;
}JobContext;


void getJobState(JobHandle job, JobState* state){
    auto jc = (JobContext*)job;
    jc->j_state.percentage = state->percentage;
    jc->j_state.stage = state->stage;
}

/**
 * locks a given mutex
 * @param mutex
 */
void lock_mutex(pthread_mutex_t *mutex){
    if (pthread_mutex_lock(mutex) != 0){
        std::cerr << "Error: cannot lock mutex" << std::endl;
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
        std::cerr << "Error: cannot unlock mutex" << std::endl;
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
    JobState new_js = {MAP_STAGE,0};
    getJobState(jc,&new_js);
//    int old_value = 0;
    int input_size = jc->input_vec.size();
    for (int i = 0; i < input_size; ++i){
        lock_mutex(&jc->mutexes.map_mutex);
        int old_value = (*(jc->jobs_atomic_count))++;
        InputPair pair = jc->input_vec[old_value];
        jc->client->map(pair.first,pair.second,arg);
        jc->j_state.percentage = 100 * old_value/input_size;
        unlock_mutex(&jc->mutexes.map_mutex);
    }

    // SORT PHASE //
    IntermediateVec curr_vec = jc->id_to_vec_map[pthread_self()];
    std::sort(curr_vec.begin(),curr_vec.end(), [](IntermediatePair p1, IntermediatePair p2){
        return *p2.first < *p1.first;
    }); // sorting according to K2 (first)
    jc->barrier->barrier();

    // SHUFFLE PHASE //
    lock_mutex(&jc->mutexes.shuffle_mutex);

    if (pthread_self()==jc->zero_thread){ // only the 0th thread shuffles.
        for(auto &elem: jc->id_to_vec_map){
            IntermediateVec cur_vec = elem.second;
            std::vector<V2> v2_vec;
            while(!cur_vec.empty()){
                IntermediatePair cur_pair = cur_vec.back();
                cur_vec.pop_back();
                if (jc->key_to_vec_map.find(cur_pair.first)==jc->key_to_vec_map.end()){ // no vector with such key
                    auto *v = new std::vector<V2*>; // creating new vector
                    jc->key_to_vec_map[cur_pair.first] = *v;
                    jc->inter_vec_vec_atomic_count ++;
                }
                jc->key_to_vec_map[cur_pair.first].push_back(cur_pair.second);  // adding the pair to the needed vector
            }
        }
    }
    unlock_mutex(&jc->mutexes.shuffle_mutex);

    jc->barrier->barrier();

    // REDUCE PHASE //
    std::vector<IntermediateVec> queue = convert_map_type(jc->key_to_vec_map);
    while(!queue.empty()){
        const IntermediateVec cur_vec = queue.back();
        jc->client->reduce(&cur_vec,jc);
    }
    return nullptr;
}
JobHandle
startMapReduceJob(const MapReduceClient &client, const InputVec &inputVec, OutputVec &outputVec, int multiThreadLevel) {
    pthread_t threads[multiThreadLevel];
    std::atomic<uint32_t> job_count(0);
    std::atomic<uint32_t> inter_vec_count(0);
    std::atomic<uint32_t> inter_vec_vec_count(0);
    std::atomic<uint32_t> out_vec_count(0);

    JobContext  job_c = {.id_to_vec_map = {},
                         .client = &client,
                         .j_state = {UNDEFINED_STAGE,0},
                         .input_vec = inputVec,
                         .out_vec = outputVec,
                         .barrier = new Barrier(multiThreadLevel),
                         .num_threads = multiThreadLevel,
                         .already_waited = false,
                         .threads = threads,
                         .jobs_atomic_count = &job_count,
                         .inter_vec_atomic_count = &inter_vec_count,
                         .inter_vec_vec_atomic_count = &inter_vec_vec_count,
                         .out_vec_atomic_count = &out_vec_count,
                         .mutexes = {PTHREAD_MUTEX_INITIALIZER,PTHREAD_MUTEX_INITIALIZER,
                                     PTHREAD_MUTEX_INITIALIZER,PTHREAD_MUTEX_INITIALIZER,
                                     PTHREAD_MUTEX_INITIALIZER},

    };

    for (int i = 0; i < multiThreadLevel; ++i) {
        pthread_create(threads + i, nullptr, thread_cycle, &job_c); // TODO handle success or fail return values
        if (i==0){
            job_c.zero_thread = pthread_self(); // distinguishing the first thread, which will do SHUFFLE
        }
    }



    return static_cast<JobHandle>(&job_c);
}
void emit2 (K2* key, V2* value, void* context){
    auto jc = (JobContext*) context;
    lock_mutex(&jc->mutexes.emit2_mutex);
    pthread_t tid = pthread_self();
    if (jc->id_to_vec_map.find(tid)==jc->id_to_vec_map.end()){ // no vector corresponds to self thread id
        auto *inter_vec = new IntermediateVec(); // creating new vector
        jc->id_to_vec_map[tid] = *inter_vec;
    }
    jc->id_to_vec_map[tid].push_back(IntermediatePair(key,value));  // adding the pair to the needed vector
    jc->inter_vec_atomic_count ++;
    unlock_mutex(&jc->mutexes.emit2_mutex);
}

void emit3 (K3* key, V3* value, void* context){
    auto jc = (JobContext*) context;
    lock_mutex(&jc->mutexes.emit3_mutex);
    jc->out_vec.push_back(OutputPair(key,value));
    jc->out_vec_atomic_count++;
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
 * closes a job - destroys mutex and frees memory
 * @param job
 */
void closeJobHandle(JobHandle job){
    auto jc = (JobContext*) job;
    waitForJob(job);
    pthread_mutex_destroy(&jc->mutexes.map_mutex);
    pthread_mutex_destroy(&jc->mutexes.emit2_mutex);
    pthread_mutex_destroy(&jc->mutexes.shuffle_mutex);
    pthread_mutex_destroy(&jc->mutexes.reduce_mutex);
    pthread_mutex_destroy(&jc->mutexes.emit3_mutex);


}
