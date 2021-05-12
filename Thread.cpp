//
// Created by hadar933 on 11/05/2021.
//

#include "Thread.h"

Thread::Thread(void (*entry_point)(void), const unsigned int id):
        _id(id), _entry_point(entry_point), _thread_status(READY){
    this->sp = (address_t)this->stack + STACK_SIZE - sizeof(address_t);
    this->pc = (address_t)entry_point;
    int ret = sigsetjmp(this->env, 1);
    (this->env->__jmpbuf)[JB_SP] = translate_address(sp);
    (this->env->__jmpbuf)[JB_PC] = translate_address(pc);
    sigemptyset(&env->__saved_mask);
    this->terminated = false;
}

Thread::~Thread() {

}

void Thread::setThreadStatus(status threadStatus) {
    _thread_status = threadStatus;
}

status Thread::getThreadStatus() const {
    return _thread_status;
}

unsigned int Thread::getId() const {
    return _id;
}

int Thread::getNumOfQuantum() const {
    return num_of_quantum;
}

bool Thread::getTerminated() const {
    return this->terminated;
}

void Thread::setTerminated(bool terminated) {
    this->terminated = terminated;
}

sigjmp_buf& Thread::getEnv() const {
    return this->env;
}



