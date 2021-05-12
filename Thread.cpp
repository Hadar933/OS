//
// Created by hadar933 on 11/05/2021.
//

#include "Thread.h"

Thread::Thread(void (*entry_point)(void), const unsigned int id):
_id(id), _entry_point(entry_point), _thread_status(READY){}


void Thread::setThreadStatus(status threadStatus) {
    _thread_status = threadStatus;
}

status Thread::getThreadStatus() const {
    return _thread_status;
}

unsigned int Thread::getId() const {
    return _id;
}
