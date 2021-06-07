//
// Created by shulik10 on 06/06/2021.
//
#include "MapReduceFramework.h"
#include <vector>
#include <map>
#include <algorithm>
#include "Barrier/Barrier.h"
//typedef std::vector<> vector;


typedef struct{
    std::map<pthread_t,IntermediateVec> id_to_vec_map; // maps thread ids to intermediate vectors
    const MapReduceClient *client;
    JobState j_state;
    const InputVec input_vec;
    OutputVec out_vec;
    Barrier *barrier;
    pthread_t zero_thread;
    std::vector<IntermediateVec> inter_vec_vec;
    std::atomic<uint64_t> inter_vec_atomic_count;
    std::atomic<uint64_t> atomic_counter; //TODO: is this initialised to zero?

}JobContext;




void getJobState(JobHandle job, JobState* state){
    auto jc = (JobContext*)job;
    jc->j_state.percentage = state->percentage;
    jc->j_state.stage = state->stage;
}
void* thread_cycle(void *arg){

    // MAP PHASE //
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
        jc->j_state.percentage = 100*  old_value/input_size;
        jc->barrier->barrier(); // unlocks mutex
    }

    // SORT PHASE //
    jc->barrier->barrier();
    IntermediateVec curr_vec = jc->id_to_vec_map[pthread_self()];
    std::sort(curr_vec.begin(),curr_vec.end()); // sorting according to K2 (first)
    jc->barrier->barrier();

    // SHUFFLE PHASE //
    if (pthread_self()==jc->zero_thread){ // only the 0th thread shuffles.
        IntermediateVec new_vec;
        for (std::pair<pthread_t, IntermediateVec> elem: jc->id_to_vec_map){
            IntermediateVec v = elem.second;
            new_vec.push_back(v.pop_back());
        }
    }

    // REDUCE PHASE //



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
                         };

    for (int i = 0; i < multiThreadLevel; ++i) {
        pthread_create(threads + i, nullptr, thread_cycle, &job_c); // TODO handle success or fail return values
        if (i==0){
            job_c.zero_thread = pthread_self(); // distinguishing the first thread, which will do SHUFFLE
        }
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
    jc->inter_vec_atomic_count ++;
    jc->barrier->barrier(); // unlocks mutex

}
void emit3 (K3* key, V3* value, void* context){

}

void waitForJob(JobHandle job){

}

void closeJobHandle(JobHandle job){
    
}
