#ifndef PROCESS_H
#define PROCESS_H

#include <stdint.h>

#define MAX_PROCESSES 16
#define STACK_SIZE    4096

#define PROC_READY    0
#define PROC_RUNNING  1
#define PROC_BLOCKED  2
#define PROC_ZOMBIE   3

//Process Control Block (PCB)
typedef struct {
    int pid;
    int state;
    uint32_t esp;          
    uint32_t eip;          
    int priority;
    char name[32];
    uint8_t stack[STACK_SIZE]; 
} pcb_t;

extern pcb_t process_table[MAX_PROCESSES];
extern pcb_t *current_process;

int create_process(char *name, void (*entry)(), int priority);
void scheduler_init(void);
void schedule(void);

#endif