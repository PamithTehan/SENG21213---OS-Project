#include "mutex.h"

static inline void cli(void) { __asm__ volatile ("cli"); }
static inline void sti(void) { __asm__ volatile ("sti"); }

void mutex_init(mutex_t *m) {
    m->locked = 0;
    m->waitq = 0;
}

void mutex_lock(mutex_t *m) {
    cli(); 

    if (m->locked) {
        current_thread->next_wait = 0;

        if (!m->waitq) {
            m->waitq = current_thread;

        } else {

            thread_t *temp = m->waitq;
            while (temp->next_wait) {
                temp = temp->next_wait;
            }
            temp->next_wait = current_thread;
        }
        
        sti();
        thread_block(); 

    } else {
        m->locked = 1;
        sti();
    }
}

void mutex_unlock(mutex_t *m) {
    cli();

    if (m->waitq != 0) {
        thread_t *w = m->waitq;
        m->waitq = m->waitq->next_wait;
        w->next_wait = 0;
        thread_unblock(w);

    } else {
        m->locked = 0;
    }

    sti();
}