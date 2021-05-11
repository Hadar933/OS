#include <iostream>
#include "uthreads.h"
#include <stdio.h>
#include <signal.h>
#include <sys/time.h>
#include "Thread.h"

#include <vector>

#define SUCCESS 0
#define FAILURE -1
#define SYSTEM_ERR "system error: "
#define THREAD_ERR "thread library error: "





Thread *allThreads[MAX_THREAD_NUM];
std::vector<Thread*> running;
std::vector<Thread*> ready;
std::vector<Thread*> blocked;





int gotit = 0 ;






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
    return SUCCESS;
}

int uthread_spawn(void (*f)(void)){
    for (unsigned int i = 0; i< MAX_THREAD_NUM; i++){
        if (allThreads[i] == nullptr){
            allThreads[i] = new Thread(f,i);
        }

    }
}
int uthread_terminate(int tid){
    return 0;
}
int uthread_block(int tid){
    return 0;
}
int uthread_resume(int tid){
    return 0;
}
int uthread_mutex_lock(){
    return 0;
}
int uthread_mutex_unlock(){
    return 0;
}
int uthread_get_tid(){
    return 0;
}
int uthread_get_total_quantums(){
    return 0;
}
int uthread_get_quantums(int tid){
    return 0;
}