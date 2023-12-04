#ifndef PCB_H
#define PCB_H

#include <stdint.h>
#include <mpx/sys_call.h>

#define STACK_SIZE 1024

// PCB Classes
#define USER_APP 0
#define SYSTEM_PROCESS 1

// PCB execution states
#define READY 0
#define BLOCKED 1

// PCB dispatching states
#define NOT_SUSPENDED 0
#define SUSPENDED 1

// PCB structure
struct pcb {
    char process_name[8];
    int process_class;
    int process_priority;
    int execution_state;
    int dispatching_state;
    unsigned char stack[STACK_SIZE];  // Allocate memory for the stack
    unsigned char *stack_ptr;  // Pointer to the top of the stack
    struct pcb *next;
};

// PCB queue structures
struct queue
{
    struct pcb *front;
};

// Function to allocate memory for a new PCB
struct pcb *allocate(void);

// Function to free memory associated with a PCB
int pcb_free(struct pcb *pcb);

// Function to allocate and initialize a new PCB
struct pcb *pcb_setup(const char *name, int process_class, int priority);

// Function to find a PCB by name
struct pcb *pcb_find(const char *name);

// Function to insert a PCB into the appropriate queue
void pcb_insert(struct pcb *pcb);

// Function to remove a PCB from its current queue
int pcb_remove(struct pcb *pcb);

// Function to set the priority of a PCB
int pcb_set_priority(const char *name, int new_priority);

void load_pcb(struct pcb *p, void (*proc)());

struct queue* get_ready_q(void);
struct queue* get_blocked_q(void);
struct queue* get_susp_ready_q(void);
struct queue* get_susp_blocked_q(void);

#endif /* PCB_H */
