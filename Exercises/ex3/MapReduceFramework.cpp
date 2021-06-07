//
// Created by shulik10 on 06/06/2021.
//
#include "MapReduceFramework.h"
#include <vector>
#include <map>
#include "Barrier/Barrier.h"
//typedef std::vector<> vector;


typedef struct{
    std::map<pthread_t,IntermediateVec> id_to_vec_map; // maps thread ids to intermediate vectors
    const MapReduceClient *client;
    JobState j_state;
    const InputVec input_vec;
    OutputVec out_vec;
    Barrier *barrier;
    int inter_vec_count;
    std::vector<IntermediateVec> queue;
    std::atomic<uint64_t> atomic_counter; //TODO: is this intialized to zero?

}JobContext;



//#################  Map Phase #################//

void getJobState(JobHandle job, JobState* state){
    auto jc = (JobContext*)job;
    jc->j_state.percentage = state->percentage;
    jc->j_state.stage = state->stage;
}
void* thread_cycle(void *arg){
    auto *jc = (JobContext*) arg;
    JobState new_js = {MAP_STAGE,0};
    getJobState(jc,&new_js);
    int old_value = 0;
    int input_size = jc->input_vec.size();
    while(old_value<= input_size){
        jc->barrier->barrier(); // locks mutex
        old_value = jc->atomic_counter++;
        InputPair pair = jc->input_vec[old_value];
        jc->client->map(pair.first,pair.second,arg);
        jc->j_state.percentage = old_value/input_size;
        jc->barrier->barrier(); // unlocks mutex
    }
    return nullptr;
}
JobHandle
startMapReduceJob(const MapReduceClient &client, const InputVec &inputVec, OutputVec &outputVec, int multiThreadLevel) {
    pthread_t threads[multiThreadLevel];

    JobContext  job_c = {.id_to_vec_map = {},
                         .client = &client,
                         .j_state = {UNDEFINED_STAGE,0},
                         .input_vec = inputVec,
                         .out_vec = outputVec,
                         .barrier = new Barrier(multiThreadLevel),
                         .inter_vec_count = 0
                         };

    for (int i = 0; i < multiThreadLevel; ++i) {
        pthread_create(threads + i, nullptr, thread_cycle, &job_c);
    }

    for (int i = 0; i < multiThreadLevel; ++i) { // wait for all threads to finish their cycle
        pthread_join(threads[i], nullptr);
    }

    return static_cast<JobHandle>(&job_c);
}
void emit2 (K2* key, V2* value, void* context){
    auto jc = (JobContext*) context;
    jc->barrier->barrier(); // locks mutex

    pthread_t tid = pthread_self();
    if (jc->id_to_vec_map.find(tid)==jc->id_to_vec_map.end()){ // no vector corresponds to self thread id
        auto *inter_vec = new IntermediateVec(); // creating new vector
        jc->id_to_vec_map[tid] = *inter_vec;
    }
    jc->id_to_vec_map[tid].push_back(IntermediatePair(key,value));  // adding the pair to the needed vector


    jc->barrier->barrier(); // unlocks mutex


}
void emit3 (K3* key, V3* value, void* context){

}

void waitForJob(JobHandle job){

}

void closeJobHandle(JobHandle job){
    
}
