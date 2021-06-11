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
#include <cmath>
#include <bitset>
#define THREAD_INIT_ERR "system error: couldn't initialize thread"
#define MUTEX_LOCK_ERR "system error: couldn't lock mutex"
#define MUTEX_UNLOCK_ERR "system error: couldn't unlock mutex"
#define THREAD_JOIN_ERR "system error: couldn't join thread"


// TODO: remove all comments

const uint64_t one_64 =1;
const uint64_t two_64 = 2;
const uint64_t thirty_three_64 = 33;
const uint64_t thirty_one_64 = 31;
const uint64_t sixty_two_64 = 62;
const int mutex_num = 9;
const int64_t first_31 = (uint64_t)1 << 31;



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
    pthread_mutex_t reduce_init_mutex;
    pthread_mutex_t wait_for_job_mutex;
    pthread_mutex_t log_print_mutex;

}mutex_struct;

/**
 * a struct containing all the context relevant to some job
 */
typedef struct JobContext{
    std::map<pthread_t,IntermediateVec*>& id_to_vec_map; // maps thread ids to intermediate vectors
    std::map<K2*,IntermediateVec>& key_to_vec_map; // maps k2 To vector of v2s (for shuffling)
    std::vector<IntermediateVec> &queue;
    std::vector<IntermediateVec*>& remember_vec;
    const MapReduceClient &client;
    JobState j_state;
    const InputVec& input_vec;
    OutputVec& out_vec;
    Barrier *barrier;
    pthread_t zero_thread;
    int num_threads;
    bool already_waited;
    bool first_iter;
    pthread_t *threads;
    uint64_t cur_inter_len;
    std::atomic<uint64_t> length_ac;
    std::atomic<uint64_t> ac;
    std::atomic<uint64_t> trying;
    mutex_struct mutexes;
}JobContext;

void update_trying(JobContext* jc, const int stage, const int jobs_todo)
{
    unsigned long long new_counter = (first_31 * stage + jobs_todo) << 31;
    jc->trying = new_counter;
}


/**
 * prints a message wrapped with a mutex
 * @param msg
 */
void print_log(std::string msg,pthread_t tid, JobContext *jc){
    pthread_mutex_lock(&jc->mutexes.log_print_mutex);
    std::cout << msg << tid << std::endl;
    pthread_mutex_unlock(&jc->mutexes.log_print_mutex);
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

    if (pthread_mutex_unlock(mutex) != 0){
        std::cerr << MUTEX_UNLOCK_ERR << std::endl;
        exit(EXIT_FAILURE);
    }
}

/**
 * assings state with the stage and percentage of the job.
 * @param job
 * @param state - new state to assign to
 */
void getJobState(JobHandle job, JobState* state) {
    auto jc = (JobContext *) job;
    unsigned long long atomic_c = jc->trying.load();
    unsigned long jobs_done = atomic_c % first_31;
    unsigned long jobs_to_do = (atomic_c << two_64) >> thirty_three_64;
    state->stage = (stage_t) (atomic_c >> sixty_two_64);
    state->percentage = ((float) jobs_done / (float)jobs_to_do)*100;
}





/**
 * converts a map of k2 keys->intermediate vec to vector of intermediate vectors
 * @param key_to_vec_map
 * @return vector of intermediate vectors
 */
void convert_map_type(JobContext* jc){
    for (auto &elem: jc->key_to_vec_map){
        IntermediateVec v;
        for (auto &pair: elem.second){
            v.push_back(pair);
        }
        jc->queue.push_back(v);
    }
}

/**
 * performs the map phase
 * @param jc
 */
void map_phase(JobContext *jc) {
    uint64_t old_value= 0;
    uint64_t z= pow(2, 31) - 1;
    int input_size = jc->input_vec.size();
    //jc->j_state = {MAP_STAGE, 0};
//    jc->ac += phase_shift; // TODO: changed from 00 to 10 instead of 01
    while ((old_value & z) < input_size-1){
        lock_mutex(&jc->mutexes.map_mutex);
        old_value = jc->ac.load();
        if((old_value & z) < input_size){
            jc->ac++;
            jc->trying += 1;
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
 * sorts the intermediate of self thread.
 * @param jc
 */
void sort_phase(JobContext *jc) {
    IntermediateVec *curr_vec = jc->id_to_vec_map[pthread_self()];
    std::sort(curr_vec->begin(),curr_vec->end());
}

/**
 * performs the shuffle phase
 * @param jc
 */
void shuffle_phase(JobContext *jc) {
    uint64_t input_size = jc->input_vec.size();
    //jc->j_state = {SHUFFLE_STAGE, 0};

    if (pthread_self()==jc->zero_thread){ // only the 0th thread shuffles.

        lock_mutex(&jc->mutexes.shuffle_mutex);
        update_trying(jc, 2, jc->id_to_vec_map.size());
        jc->ac -= input_size;
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
                        jc->remember_vec.push_back(new_vec);
                        new_vec->push_back(cur_pair);
                        (jc->ac)++;  // count number of vectors in queue
                        jc->key_to_vec_map[cur_pair.first] = *new_vec;
                    }
                }
            }
            jc->trying++;
        }
        update_trying(jc, 3, jc->key_to_vec_map.size());
        unlock_mutex(&jc->mutexes.shuffle_mutex);
    }
}

/**
 * performs the reduce phase
 * @param jc
 */
void reduce_phase(JobContext *jc) {

    //jc->j_state = {REDUCE_STAGE, 0};
//    jc->ac += phase_shift;
    lock_mutex(&jc->mutexes.reduce_mutex);
    if (jc->first_iter){
        jc->first_iter = false;
        uint64_t size = ((jc->ac << two_64) >> thirty_three_64) << thirty_one_64; // setting the middle section to zero
        jc->ac -= size;
        convert_map_type(jc);
    }
    unlock_mutex(&jc->mutexes.reduce_mutex);

    lock_mutex(&jc->mutexes.reduce_mutex);

    uint64_t z= pow(2, 31) - 1;
    while((jc->ac << two_64) >> thirty_three_64 < ((jc->ac)&z)){ // mid ac smaller then right part (queue size)
        int q_size = jc->queue.size();
        if(q_size == 0){ break;}
        IntermediateVec v;
        v = jc->queue[q_size-1];
        jc->cur_inter_len = v.size();
        jc->queue.pop_back();
        jc->trying ++;
        jc->ac += one_64 << thirty_one_64; // +1 to middle section
        jc->client.reduce(&v,jc);
    }
    unlock_mutex(&jc->mutexes.reduce_mutex);
}


/**
 * performs an entire thread life cycle
 * @param arg the context (casted to job handler)
 * @return
 */
void* thread_cycle(void *arg){

    auto *jc = (JobContext*) arg;
    lock_mutex(&jc->mutexes.cycle_mutex);
    auto *cur_vec = new std::vector<IntermediatePair>();
    if(jc->id_to_vec_map.empty()){
        jc->zero_thread = pthread_self();
    }
    if(jc->trying >> 62 == 0){ update_trying(jc, 1, jc->input_vec.size());}
    jc->id_to_vec_map.insert({pthread_self(),cur_vec}); // adding zero thread
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
    pthread_t *threads = (pthread_t*) malloc(sizeof(pthread_t)*(multiThreadLevel) );
    auto id_to_vec_map = new std::map<pthread_t,IntermediateVec*>;
    auto key_to_vec_map = new std::map<K2*,IntermediateVec>;
    auto remember_vec = new std::vector<IntermediateVec*>;
    auto queue = new std::vector<IntermediateVec>;
    for(int i=0; i< mutex_num; i++){
        mutexes[i] = PTHREAD_MUTEX_INITIALIZER;
    }
    JobContext *job_c = new JobContext {
            .id_to_vec_map = *id_to_vec_map,
            .key_to_vec_map = *key_to_vec_map,
            .queue = *queue,
            .remember_vec = *remember_vec,
            .client = client,
            .j_state = {UNDEFINED_STAGE,0},
            .input_vec = inputVec,
            .out_vec = outputVec,
            .barrier = new Barrier(multiThreadLevel),
            .num_threads = multiThreadLevel,
            .already_waited = false,
            .first_iter = true,
            .threads = threads,
            .cur_inter_len = 0,
            .length_ac{0},
            .ac{0},
            .trying{0},
            .mutexes = {
                    .map_mutex = mutexes[0],
                    .emit2_mutex = mutexes[1],
                    .shuffle_mutex =mutexes[2],
                    .reduce_mutex = mutexes[3],
                    .emit3_mutex = mutexes[4],
                    .cycle_mutex = mutexes[5],
                    .reduce_init_mutex = mutexes[6],
                    .wait_for_job_mutex = mutexes[7],
                    .log_print_mutex = mutexes[8]
            }
    };

    pthread_mutex_t init_mutex = PTHREAD_MUTEX_INITIALIZER;
    lock_mutex(&init_mutex);
    for (int i = 0; i < multiThreadLevel; ++i){
        if(pthread_create(&threads[i], nullptr, thread_cycle, job_c)!=0){
            std::cerr << THREAD_INIT_ERR << std::endl;
            exit(EXIT_FAILURE);
        }
    }
    unlock_mutex(&init_mutex);
    for(int i=0; i < mutex_num;i++){
        pthread_mutex_destroy(&mutexes[i]);
    }
    free(mutexes);mutexes = nullptr;
    auto jc = static_cast<JobHandle>(job_c);
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
    uint64_t inc = one_64 << thirty_one_64;
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
    uint64_t cur_shifter_len = jc->cur_inter_len ;
    jc->length_ac += cur_shifter_len;
    unlock_mutex(&jc->mutexes.emit3_mutex);
}

/**
 * uses pthread_join to wait for jobs.
 * @param job
 */
void waitForJob(JobHandle job) {
    auto jc = (JobContext *) job;
    lock_mutex(&jc->mutexes.wait_for_job_mutex);

    if (!jc->already_waited) {
        for (int i = 0; i < jc->num_threads; ++i) {
                pthread_t tid = jc->threads[i];
                int ret = pthread_join(tid, nullptr);
                if (ret!=0){
                    std::cout <<THREAD_JOIN_ERR << " " << tid << "" << ret << std::endl;
                    exit(EXIT_FAILURE);
                }
        }
        jc->already_waited = true;
    }
    unlock_mutex(&jc->mutexes.wait_for_job_mutex);

}

/**
 * closes a job - destroys mutex and frees memory
 * @param job
 */
void closeJobHandle(JobHandle job){
    waitForJob(job);
    auto jc = (JobContext*) job;
    delete jc->barrier;
    for (auto &elem: jc->id_to_vec_map){
        delete elem.second;
    }
    for (auto elem: jc->remember_vec){
        delete elem;
    }
    free(jc->threads); jc->threads = nullptr;

    delete &jc->remember_vec;
    delete &jc->id_to_vec_map;
    delete &jc->key_to_vec_map;
    delete &jc->queue;
    delete jc;

}