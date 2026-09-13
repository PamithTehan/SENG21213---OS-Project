#include "mutex.h"
#include "irq.h"

void mutex_init(mutex_t *m) {
    m->locked = 0;
    m->waitq = 0;
}

void mutex_lock(mutex_t *m) {
    uint32_t flags = irq_save(); 

    if (m->locked) {
        if (current_thread) {
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
            
            irq_restore(flags);
            thread_block(); 
            return;
        }
    } else {
        m->locked = 1;
    }

    irq_restore(flags); 
}

void mutex_unlock(mutex_t *m) {
    uint32_t flags = irq_save();

    if (m->waitq != 0) {
        thread_t *w = m->waitq;
        m->waitq = m->waitq->next_wait;
        w->next_wait = 0;
        thread_unblock(w);
    } else {
        m->locked = 0;
    }

    irq_restore(flags);
}