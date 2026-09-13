#include "process.h"

extern void context_switch(pcb_t *cur, pcb_t *next);

static int current_index = 0;

void schedule(void) {
    if (!current_process) return;

    int next_index = -1;
    for (int i = 1; i <= MAX_PROCESSES; i++) {
        
        int idx = (current_index + i) % MAX_PROCESSES;
        if (process_table[idx].pid != 0 && process_table[idx].state == PROC_READY) {
            next_index = idx;
            break;
        }
    }

    if (next_index == -1) {
        return;
    }

    pcb_t *cur = current_process;
    pcb_t *next = &process_table[next_index];

    if (cur->state == PROC_RUNNING) {
        cur->state = PROC_READY;
    }
    next->state = PROC_RUNNING;
    current_index = next_index;
    current_process = next;

    context_switch(cur, next);
}