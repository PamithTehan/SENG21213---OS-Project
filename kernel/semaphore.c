#include "semaphore.h"
#include "irq.h"

void sem_init(sem_t *s, int init_count) {
    s->count = init_count;
    s->waitq = 0;
}

void sem_wait(sem_t *s) {
    uint32_t flags = irq_save();
    s->count--;

    if (s->count < 0) {
        if (current_thread) {
            current_thread->next_wait = 0;
            if (!s->waitq) {
                s->waitq = current_thread;
            } else {
                thread_t *curr = s->waitq;
                while (curr->next_wait) {
                    curr = curr->next_wait;
                }
                curr->next_wait = current_thread;
            }

            irq_restore(flags);
            thread_block();
            return;
        }
    }

    irq_restore(flags);
}

void sem_signal(sem_t *s) {
    uint32_t flags = irq_save();
    s->count++;

    if (s->count <= 0) {
        if (s->waitq) {
            thread_t *w = s->waitq;
            s->waitq = s->waitq->next_wait;
            w->next_wait = 0;
            thread_unblock(w);
        }
    }
    
    irq_restore(flags);
}