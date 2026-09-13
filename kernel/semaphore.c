#include "semaphore.h"

static inline void cli(void) { __asm__ volatile ("cli"); }
static inline void sti(void) { __asm__ volatile ("sti"); }

void sem_init(sem_t *s, int init_count) {
    s->count = init_count;
    s->waitq = 0;
}

void sem_wait(sem_t *s) {
    cli();
    s->count--;

    if (s->count < 0) {
        
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

        sti();
        thread_block();

    } else {
        sti();
    }
}

void sem_signal(sem_t *s) {
    cli();
    s->count++;

    if (s->count <= 0) {

        if (s->waitq) {
            thread_t *w = s->waitq;
            s->waitq = s->waitq->next_wait;
            w->next_wait = 0;
            thread_unblock(w);
        }
    }
    sti();
}