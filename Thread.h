//
// Created by hadar933 on 11/05/2021.
//

#include "uthreads.h"

#ifndef EX2_THREAD_H
#define EX2_THREAD_H

enum status{
    READY,BLOCKED,RUNNING
};

class Thread {
public:
    Thread(void (*entryPoint)(void),unsigned int id);

    status getThreadStatus() const;

private:
public:
    void setThreadStatus(status threadStatus);

    unsigned int getId() const;

    bool isWaitingForMutex() const;

    void setWaitingForMutex(bool waitingForMutex);

private:
    unsigned int _id;
    void (*_entry_point)(void);
    char stack[STACK_SIZE];
    status _thread_status;
    bool _waiting_for_mutex;
};


#endif //EX2_THREAD_H
