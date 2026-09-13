#ifndef THREAD_H
#define THREAD_H

#include <stdint.h>
#include "process.h"

#define THREAD_READY    0
#define THREAD_RUNNING  1
#define THREAD_BLOCKED  2
#define THREAD_EXIT     3

#define THREAD_STACK_SIZE 1024
#define MAX_THREADS       16

typedef struct thread {
    int tid;
    int state;
    uint32_t esp;
    uint32_t eip;
    pcb_t *parent;
    uint32_t stack[THREAD_STACK_SIZE];
    struct thread *next_wait; 
} thread_t;

extern thread_t *current_thread;

void thread_init(void);

thread_t *create_thread(void (*entry)(void), pcb_t *parent);

void thread_block(void);

void thread_unblock(thread_t *t);

#endif