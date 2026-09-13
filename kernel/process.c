#include "process.h"

pcb_t process_table[MAX_PROCESSES];
pcb_t *current_process = 0;

static int next_pid = 1;

int create_process(char *name, void (*entry)(), int priority) {

    for (int i = 0; i < MAX_PROCESSES; i++) {

        if (process_table[i].state == PROC_ZOMBIE || process_table[i].pid == 0) {
            
            pcb_t *p = &process_table[i];
            
            p->pid = next_pid++;
            p->state = PROC_READY;
            p->priority = priority;
            p->eip = (uint32_t)entry;

            int j = 0;
            while (name[j] && j < 31) {
                 p->name[j] = name[j]; j++; 
                }
            p->name[j] = '\0';

            uint32_t *stk = (uint32_t *)(p->stack + STACK_SIZE);

            *(--stk) = (uint32_t)entry;     
            *(--stk) = 0x00000202;           
            
            for (int k = 0; k < 8; k++) {
                *(--stk) = 0;
            }

            p->esp = (uint32_t)stk;         
            return p->pid;
        }
    }
    return -1; 
}