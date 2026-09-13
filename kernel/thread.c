#include "thread.h"

static thread_t thread_table[MAX_THREADS];
thread_t *current_thread = 0;
static int next_tid = 1;

void thread_init(void) {

    for (int i = 0; i < MAX_THREADS; i++) {
        thread_table[i].tid = 0;
        thread_table[i].state = THREAD_EXIT;
        thread_table[i].next_wait = 0;
    }
}

thread_t *create_thread(void (*entry)(void), pcb_t *parent) {

    for (int i = 0; i < MAX_THREADS; i++) {

        if (thread_table[i].state == THREAD_EXIT || thread_table[i].tid == 0) {
            thread_t *t = &thread_table[i];
            t->tid = next_tid++;
            t->state = THREAD_READY;
            t->parent = parent;
            t->eip = (uint32_t)entry;
            t->next_wait = 0;

            uint32_t *stk = (uint32_t *)(t->stack + THREAD_STACK_SIZE);

            *(--stk) = (uint32_t)entry;     
            *(--stk) = 0x00000202;           
            for (int k = 0; k < 8; k++) {    
                *(--stk) = 0;
            }

            t->esp = (uint32_t)stk;
            return t;
        }
    }
    return 0;
}

void thread_block(void) {

    if (current_thread) {
        current_thread->state = THREAD_BLOCKED;
        schedule();
    }
}

void thread_unblock(thread_t *t) {
    
    if (t) {
        t->state = THREAD_READY;
    }
}