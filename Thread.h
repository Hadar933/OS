//
// Created by hadar933 on 11/05/2021.
//

#include "uthreads.h"
#include <setjmp.h>
#include <signal.h>
#include "unistd.h"

#ifndef EX2_THREAD_H
#define EX2_THREAD_H
typedef unsigned long address_t;

#ifdef __x86_64__
/* code for 64 bit Intel arch */

typedef unsigned long address_t;
#define JB_SP 6
#define JB_PC 7

/* A translation is required when using an address of a variable.*/
address_t translate_address(address_t addr);

#endif

enum status{
    READY,BLOCKED,RUNNING
};

class Thread {
public:
    Thread(void (*entryPoint)(void),unsigned int id);
    int getNumOfQuantum() const;
    status getThreadStatus() const;
    void inc_quantum(){++(this->num_of_quantum);};
    void setThreadStatus(status threadStatus);
    bool getTerminated()const;
    unsigned int getId() const;

private:
    unsigned int _id;
    void (*_entry_point)(void);
    char stack[STACK_SIZE];
    status _thread_status;
    address_t sp;
    address_t pc;
    sigjmp_buf env;
    int num_of_quantum;
    bool terminated;
public:
    sigjmp_buf& getEnv();

    void setTerminated(bool terminated);

};


#endif //EX2_THREAD_H
