//
// Created by shulik10 on 06/06/2021.
//
#include "MapReduceFramework.h"
#include <vector>

//typedef std::vector<> vector;

typedef struct{
    JobState j_state;
    IntermediateVec *int_vec;
    InputVec input_vec;
    OutputVec out_vec;
    std::atomic<uint64_t> atomic_counter;
}JobContext;


//#################  Map Phase #################//
void* thread_cycle(void* garbage){

    return nullptr;
}
JobHandle
startMapReduceJob(const MapReduceClient &client, const InputVec &inputVec, OutputVec &outputVec, int multiThreadLevel) {
    pthread_t threads[multiThreadLevel];
    JobContext job_c = {{UNDEFINED_STAGE, 0}, new IntermediateVec, inputVec, outputVec};
    for (int i = 0; i < multiThreadLevel; ++i) {
        pthread_create(threads + i, NULL, thread_cycle, NULL);
    }
}




