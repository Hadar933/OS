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

Thread *running_thread;

int mutex = -1;
int total_quantums = 0;
sigset_t sig_set;


#define JB_SP 6
#define JB_PC 7

/**
 * looking for some element item in some vector lst
 * @param lst
 * @param item
 * @return index
 */
int look_for(std::vector<Thread*> lst, Thread *item){
    for (unsigned long i=0;i<lst.size();i++){
        if (item == lst[i]){
            return (int)i;
        }
    }
    return FAILURE;
}

void restore_blocked_by_m(){
    Thread *last_thread = blocked_by_mutex.front();
    blocked_by_mutex.erase(blocked_by_mutex.begin()); // extracting some thread (specifically the last one)
    if (look_for(blocked, last_thread) == FAILURE) { // only blocked my mutex (not blocked my main)
        ready.push_back(last_thread);
        last_thread->setThreadStatus(READY);
    }
}

void scheduler(int sig)
{   // only save environment if running thread is not blocked or terminated
    if (!(running_thread->getThreadStatus() == BLOCKED || running_thread->getTerminated())){
        int ret_val = sigsetjmp(running_thread->getEnv(), 1);//save state
        if(ret_val==1){
            return;
        }
        if(!ready.empty()){
            ready.push_back(running_thread);
            running_thread->setThreadStatus(READY);
        }
    }
    if(!ready.empty()){
        Thread *first = ready[0];
        ready.erase(std::remove(ready.begin(),ready.end(),first),ready.end()); // remove from ready lst
        if (running_thread->getTerminated()){
            all_threads[running_thread->getId()] = nullptr;
            if(mutex == (int)running_thread->getId()){
                if(!blocked_by_mutex.empty()){
                    restore_blocked_by_m();
                }
                mutex = UNLOCKED;
            }
            delete running_thread;
        }
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

int uthread_init(int quantum_usecs){
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

    Thread *main_thread = new Thread(nullptr,0);
    all_threads[0] = main_thread;
    running_thread = main_thread;
    main_thread->inc_quantum();
    total_quantums++;
    int ret_val = sigsetjmp(main_thread->getEnv(),1);
    if (ret_val==1){
        return SUCCESS;
    }
    return SUCCESS;
}

int uthread_spawn(void (*f)(void)){
    BLOCK_SIG;
    for (unsigned int i = 0; i< MAX_THREAD_NUM; i++){
        if (all_threads[i] == nullptr){
            Thread *new_thread = new Thread(f,i);
            all_threads[i] = new_thread;
            ready.push_back(new_thread);
            ALLOW_SIG;
            return i;
        }
    }
    ALLOW_SIG;
    std::cerr << THREAD_ERR << "max occupancy reached" << std::endl;
    return FAILURE;
}

int uthread_terminate(int tid){
    BLOCK_SIG;
    if(all_threads[tid]== nullptr){ // to thread with id = tid
        std::cerr << THREAD_ERR<< "thread does not exist" << std::endl;
        ALLOW_SIG;
        return FAILURE;
    }
    if(tid == 0){ // terminating main thread
        ready.clear();
        blocked.clear();
        blocked_by_mutex.clear();
        delete [] *all_threads;
        exit(EXIT_SUCCESS);
    }
    if(tid == (int)running_thread->getId()){
        ALLOW_SIG;
        running_thread->setTerminated(true);
        scheduler(0);
    }
    /* thread exits - delete it*/
    Thread *to_terminate = all_threads[tid];
    if(to_terminate->getThreadStatus() == READY) {
        ready.erase(std::remove(ready.begin(), ready.end(), to_terminate), ready.end());
    }
    else if(to_terminate->getThreadStatus() == BLOCKED){
        if (look_for(blocked_by_mutex,to_terminate)!= FAILURE){
            blocked_by_mutex.erase(std::remove(blocked_by_mutex.begin(),
                                               blocked_by_mutex.end(), to_terminate), blocked_by_mutex.end());
        }
        if (look_for(blocked,to_terminate) != FAILURE){
            blocked.erase(std::remove(blocked.begin(), blocked.end(), to_terminate), blocked.end());
        }
    }
    delete to_terminate;
    all_threads[tid] = nullptr;
    if (mutex == tid){
        mutex = UNLOCKED;
    }
    ALLOW_SIG;
    return 0;
}
int uthread_block(int tid){
    BLOCK_SIG;
    if (tid==0){
        std::cerr << THREAD_ERR << "cannot block main thread" << std::endl;
        ALLOW_SIG;
        return FAILURE;
    }
    Thread *thread_tid = all_threads[tid];
    if(tid == (int)running_thread->getId()){
        int ret_val = sigsetjmp(running_thread->getEnv(), 1);//save state
        if(ret_val==1){
            ALLOW_SIG;
            return SUCCESS;
        }
        running_thread->setThreadStatus(BLOCKED);
        blocked.push_back(running_thread);
        scheduler(0);
    }

    if (thread_tid != nullptr){ // thread exists
        if(thread_tid->getThreadStatus()==READY){ // is ready
            for(unsigned long i=0;i<ready.size();i++){
                Thread *temp_t = ready[i];
                if ((int)temp_t->getId()==tid){
                    ready.erase(std::remove(ready.begin(),ready.end(),temp_t),ready.end()); // removing ready[i]
                    blocked.push_back(temp_t); // adding to blocked
                    break;
                }
            }
        }
        if (look_for(blocked_by_mutex,thread_tid)!= FAILURE){
            blocked.push_back(thread_tid);
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
        if (look_for(blocked_by_mutex,thread_tid)!=FAILURE){
            ALLOW_SIG;
            return SUCCESS;
        }
        ready.push_back(thread_tid); // not locked by mutex - can be ready
        thread_tid->setThreadStatus(READY);
        ALLOW_SIG;
        return SUCCESS;
    }
    ALLOW_SIG;
    std::cerr << THREAD_ERR << "no such thread" << std::endl;
    return FAILURE;
}
int uthread_mutex_lock(){
    BLOCK_SIG;
    // if mutex == -1 we set it to the id of the running thread.
    // otherwise we lock the running thread (lock by mutex)

    if(mutex == (int)running_thread->getId()){
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
        int ret_val = sigsetjmp(running_thread->getEnv(),1);
        if (ret_val == 1){
            mutex = running_thread->getId();
            ALLOW_SIG;
            return SUCCESS;
        }
        scheduler(0);
    }
    ALLOW_SIG;
    return SUCCESS;

}

int uthread_mutex_unlock() {
    BLOCK_SIG;

    if (mutex == UNLOCKED || (int)running_thread->getId() != mutex) {
        std::cerr << THREAD_ERR << "mutex already unlocked" << std::endl;
        ALLOW_SIG;
        return FAILURE;
    }
    if (!blocked_by_mutex.empty()) { // there are blocked threads waiting for this mutex
        restore_blocked_by_m();
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
   if (all_threads[tid] != nullptr){
       return all_threads[tid]->getNumOfQuantum();
   }
   std::cerr << THREAD_ERR << "thread" << tid << "does not exist (no get quantums)" << std::endl;
   return FAILURE;
}