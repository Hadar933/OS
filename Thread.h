//
// Created by hadar933 on 11/05/2021.
//

#include "uthreads.h"

#ifndef EX2_THREAD_H
#define EX2_THREAD_H


class Thread {
public:
    Thread(void (*entryPoint)(void),unsigned int id);

private:
    unsigned int _id;
    void (*_entry_point)(void);
    char stack[STACK_SIZE];
};


#endif //EX2_THREAD_H
