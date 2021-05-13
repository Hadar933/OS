//
// Created by hadar933 on 11/05/2021.
//

#include "Thread.h"


address_t translate_address(address_t addr){
    address_t ret;
    asm volatile("xor    %%fs:0x30,%0\n"
                 "rol    $0x11,%0\n"
    : "=g" (ret)
    : "0" (addr));
    return ret;
}

Thread::Thread(void (*entry_point)(void), const unsigned int id):
        _id(id), _entry_point(entry_point), _thread_status(READY){
    if (id==0){
        this->_thread_status = RUNNING; // main thread
    }
    this->sp = (address_t)this->stack + STACK_SIZE - sizeof(address_t);
    this->pc = (address_t)entry_point;
    int ret = sigsetjmp(this->env, 1);
    (this->env->__jmpbuf)[JB_SP] = translate_address(sp);
    (this->env->__jmpbuf)[JB_PC] = translate_address(pc);
    sigemptyset(&env->__saved_mask);
    this->terminated = false;
}

void Thread::setTerminated(bool t) {
    Thread::terminated = t;
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


sigjmp_buf& Thread::getEnv() {
    return this->env;
}



