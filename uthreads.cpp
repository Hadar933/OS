#include <iostream>
#include "uthreads.h"
#include <stdio.h>
#include <signal.h>
#include <setjmp.h>
#include <sys/time.h>
#include "Thread.h"

#include <algorithm>
#include <vector>

#define UNLOCKED -1
#define SUCCESS 0
#define FAILURE -1
#define SYSTEM_ERR "system error: "
#define THREAD_ERR "thread library error: "
#define BLOCK_SIG sigprocmask(SIG_BLOCK, &sig_set,NULL)
#define ALLOW_SIG sigprocmask(SIG_UNBLOCK, &sig_set,NULL)


Thread *all_threads[MAX_THREAD_NUM];
std::vector<Thread*> ready;
std::vector<Thread*> blocked;
std::vector<Thread*> blocked_by_mutex;

Thread *running_thread; // TODO: this must be updated when ever some thread is switched to running ? ?

int gotit = 0;
int mutex = -1;
int total_quantums = 0;
sigset_t sig_set;


#define JB_SP 6
#define JB_PC 7


void scheduler(int sig)
{

    int ret_val = sigsetjmp(running_thread->getEnv(), 1);//save state
    if(ret_val==1){
        return;
    }
    if(!ready.empty()){
        ready.push_back(running_thread);
        running_thread->setThreadStatus(READY);
    }

    if (!ready.empty()){
        Thread *first = ready[0];
        ready.erase(std::remove(ready.begin(),ready.end(),first),ready.end()); // remove from ready lst
        first->setThreadStatus(RUNNING);
        running_thread = first;
        running_thread->inc_quantum();
        total_quantums++;
        siglongjmp(running_thread->getEnv(), 1);
    }
    else{ // ready vec is empty, running thread stays the same
        running_thread->setThreadStatus(RUNNING);
        running_thread->inc_quantum();
        total_quantums++;
    }

}

int uthreads_init(int quantum_usecs){
    if (quantum_usecs<=0){
        std::cerr <<THREAD_ERR<<"quantum_usecs must be > 0"<< std::endl;
        return FAILURE;
    }
    struct sigaction sa = {nullptr};
    struct itimerval timer{};

    // Install scheduler as the signal handler for SIGVTALRM.
    sa.sa_handler = &scheduler;
    if (sigaction(SIGVTALRM, &sa,nullptr) < 0) {
        std::cerr <<SYSTEM_ERR<< "sigaction error." << std::endl;
        exit(1);
    }
    sigemptyset(&sig_set);
    sigaddset(&sig_set,SIGVTALRM);

    timer.it_value.tv_sec = 0;
    timer.it_value.tv_usec = quantum_usecs;

    timer.it_interval.tv_sec = 0;	// following time intervals, seconds part
	timer.it_interval.tv_usec = quantum_usecs;	// following time intervals, microseconds part

    if (setitimer (ITIMER_VIRTUAL, &timer, nullptr)) {
        std::cerr <<SYSTEM_ERR<< "setitimer error." << std::endl;
        exit(1);
    }

    all_threads[0] = new Thread(nullptr,0); // main thread
    running_thread = all_threads[0];
    return SUCCESS;
}

int uthread_spawn(void (*f)(void)){
    BLOCK_SIG;
    for (unsigned int i = 0; i< MAX_THREAD_NUM; i++){
        if (all_threads[i] == nullptr){
            Thread *new_thread = new Thread(f,i);
            all_threads[i] = new_thread;
            ready.push_back(new_thread);
            return i;
        }
    }
    ALLOW_SIG;
    return FAILURE;
}

int uthread_terminate(int tid){
    BLOCK_SIG;
    if(all_threads[tid]== nullptr){ // to thread with id = tid
        std::cerr << THREAD_ERR << "thread does not exist" << std::endl;
    }
    ALLOW_SIG;

}
int uthread_block(int tid){
    BLOCK_SIG;
    Thread *thread_tid = all_threads[tid];
    if(tid == running_thread->getId()){
        scheduler(0); //TODO: ??? dont know yet
    }
    if (tid==0){
        std::cerr << THREAD_ERR << "cannot block main thread" << std::endl;
        ALLOW_SIG;
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
        ALLOW_SIG;
        return SUCCESS;
    }
    std::cerr << THREAD_ERR << "no such thread" << std::endl;
    ALLOW_SIG;
    return FAILURE;
}
int uthread_resume(int tid){
    BLOCK_SIG;
    Thread *thread_tid = all_threads[tid];
    if(thread_tid != nullptr){ // thread exists
        status s = thread_tid->getThreadStatus();
        if (s == RUNNING || s == READY){ // thread is not blocked
            ALLOW_SIG;
            return SUCCESS;
        }
        blocked.erase(std::remove(blocked.begin(),blocked.end(),thread_tid),blocked.end()); // removing from blocked
        for (int i = 0 ; i < blocked_by_mutex.size(); i++){ // checking if blocked by mutex
            if (blocked_by_mutex[i]==thread_tid){
                ALLOW_SIG;
                return SUCCESS;
            }
        }
        ready.push_back(thread_tid); // not locked by mutex - can be ready
        thread_tid->setThreadStatus(READY);
        ALLOW_SIG;
        return SUCCESS;
    }
    ALLOW_SIG;
    return FAILURE;
}
int uthread_mutex_lock(){
    BLOCK_SIG;
    // if mutex == -1 we set it to the id of the running thread.
    // otherwise we lock the running thread (lock by mutex)
    if(mutex == running_thread->getId()){
        std::cerr << THREAD_ERR << "mutex already locked by running thread" << std::endl;
        ALLOW_SIG;
        return FAILURE;
    }
    else if (mutex == UNLOCKED){
        mutex = running_thread->getId();
    }
    else{ // LOCKED
        blocked_by_mutex.push_back(running_thread);
        running_thread->setThreadStatus(BLOCKED);
    }
    ALLOW_SIG;
    return SUCCESS;

}
int uthread_mutex_unlock(){
    BLOCK_SIG;
    if (mutex == UNLOCKED){
        std::cerr << THREAD_ERR << "mutex already unlocked" << std::endl;
        ALLOW_SIG;
        return FAILURE;
    }
    if (!blocked_by_mutex.empty()){ // there are blocked threads waiting for this mutex
        Thread *last_thread = blocked_by_mutex.back();
        blocked_by_mutex.pop_back(); // extracting some thread (specifically the last one)
        for (int i = 0; i < blocked.size() ; i++){
            if (blocked[i]==last_thread){ // the thread is blocked by main- we cannot turn it to ready
                mutex = UNLOCKED;
                ALLOW_SIG;
                return SUCCESS;
            }
        }
        ready.push_back(last_thread);
        last_thread->setThreadStatus(READY);
        mutex = UNLOCKED;
        ALLOW_SIG;
        return SUCCESS;
    }
    mutex = UNLOCKED;
    ALLOW_SIG;
    return SUCCESS;

}
int uthread_get_tid(){
    return running_thread->getId();
}
int uthread_get_total_quantums(){
    return total_quantums;
}
int uthread_get_quantums(int tid){
   return all_threads[tid]->getNumOfQuantum();

}