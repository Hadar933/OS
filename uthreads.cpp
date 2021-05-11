#include <iostream>
#include "uthreads.h"
#include <stdio.h>
#include <signal.h>
#include <sys/time.h>
#include "Thread.h"

#include <algorithm>
#include <vector>

#define UNLOCKED -1
#define SUCCESS 0
#define FAILURE -1
#define SYSTEM_ERR "system error: "
#define THREAD_ERR "thread library error: "


Thread *all_threads[MAX_THREAD_NUM];
std::vector<Thread*> ready;
std::vector<Thread*> blocked;
Thread *running_thread; // TODO: this must be updated when ever some thread is switched to running ? ?

int gotit = 0;
int mutex = -1;


void timer_handler(int sig)
{
    gotit = 1;
    printf("Timer expired\n");
}

int uthreads_init(int quantum_usecs){
    if (quantum_usecs<=0){
        std::cerr <<THREAD_ERR<<"quantum_usecs must be > 0"<< std::endl;
        return FAILURE;
    }
    struct sigaction sa = {nullptr};
    struct itimerval timer{};

    // Install timer_handler as the signal handler for SIGVTALRM.
    sa.sa_handler = &timer_handler;
    if (sigaction(SIGVTALRM, &sa,nullptr) < 0) {
        std::cerr <<SYSTEM_ERR<< "sigaction error." << std::endl;
        exit(1);
    }
    timer.it_value.tv_sec = 0;
    timer.it_value.tv_usec = quantum_usecs;
    if (setitimer (ITIMER_VIRTUAL, &timer, nullptr)) {
        std::cerr <<SYSTEM_ERR<< "setitimer error." << std::endl;
        exit(1);
    }

    all_threads[0] = new Thread(nullptr,0); // main thread
    running_thread = all_threads[0];
    return SUCCESS;
}

int uthread_spawn(void (*f)(void)){
    for (unsigned int i = 0; i< MAX_THREAD_NUM; i++){
        if (all_threads[i] == nullptr){
            Thread *new_thread = new Thread(f,i);
            all_threads[i] = new_thread;
            ready.push_back(new_thread);
            return i;
        }
    }
    return FAILURE;
}

int uthread_terminate(int tid){
    return 0;

}
int uthread_block(int tid){
    Thread *thread_tid = all_threads[tid];
    if(tid == running_thread->getId()){
        timer_handler(0); //TODO: ??? dont know yet
    }
    if (tid==0){
        std::cerr << THREAD_ERR << "cannot block main thread" << std::endl;
        return FAILURE;
    }
    if (thread_tid != nullptr){ // thread exists
        if(thread_tid->getThreadStatus()==READY){ // is ready
            for(int i=0;i<ready.size();i++){
                Thread *temp_t = ready[i];
                if (temp_t->getId()==tid){
                    ready.erase(std::remove(ready.begin(),ready.end(),temp_t),ready.end()); // removing ready[i]
                    blocked.push_back(temp_t); // adding to blocked
                    break;
                }
            }
        }
        thread_tid->setThreadStatus(BLOCKED);
        return SUCCESS;
    }
    std::cerr << THREAD_ERR << "no such thread" << std::endl;
    return FAILURE;
}
int uthread_resume(int tid){
    Thread *thread_tid = all_threads[tid];
    if(thread_tid != nullptr){ // thread exists
        status s = thread_tid->getThreadStatus();
        if (s == RUNNING || s == READY){ // thread is not blocked
            return SUCCESS;
        }
        if (thread_tid->isWaitingForMutex()){
            return SUCCESS;
        }
        blocked.erase(std::remove(blocked.begin(),blocked.end(),thread_tid),blocked.end()); // removing from blocked
        ready.push_back(thread_tid); // adding to ready
        thread_tid->setThreadStatus(READY);
        return SUCCESS;
    }
    return FAILURE;
}
int uthread_mutex_lock(){
    if (mutex == UNLOCKED){
        mutex = running_thread->getId();
        return SUCCESS;
    }
    else{ // LOCKED
        running_thread->setWaitingForMutex(true);
        uthread_block(running_thread->getId());


    }
}
int uthread_mutex_unlock(){
    return 0;
}
int uthread_get_tid(){
    return running_thread->getId();
}
int uthread_get_total_quantums(){
    return 0;
}
int uthread_get_quantums(int tid){
    return 0;
}