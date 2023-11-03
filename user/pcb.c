#include <string.h>
#include <pcb.h>
#include <memory.h>
#include <sys_req.h>

#define STACK_SIZE 1024

static struct queue *ready_q = NULL;
static struct queue *blocked_q = NULL;
static struct queue *susp_ready_q = NULL;
static struct queue *susp_blocked_q = NULL;

// Function to allocate memory for a new PCB
struct pcb *allocate(void) {
    struct pcb *new_pcb = (struct pcb *) sys_alloc_mem(sizeof(struct pcb));

    new_pcb->stack_ptr = (unsigned char*)sys_alloc_mem((size_t)1024);   // allocates memory for the pcb stack
    
    return new_pcb;
}


int pcb_free(struct pcb *pcb) {
    if (pcb != NULL) {
        sys_free_mem(pcb);
        return 0;
    }
    return -1;
}



// Function to allocate and initialize a new PCB
struct pcb *pcb_setup(const char *name, int process_class, int priority) {
    struct pcb *new_pcb = allocate();
    if (new_pcb != NULL) {
        strcpy(new_pcb->process_name, name);
        new_pcb->process_class = process_class;
        new_pcb->process_priority = priority;
        new_pcb->execution_state = READY;
        new_pcb->dispatching_state = NOT_SUSPENDED;
       // new_pcb->stack_ptr->eip = (uint32_t) function;
    }
    return new_pcb;
}


// Function to find a PCB by name
struct pcb *pcb_find(const char *name)
{
    struct pcb *current;
    // Search Ready Queue if it exists
    if (ready_q != NULL)
    {
        current = ready_q->front;   // start with the pcb in the front

        // Loop through all PCB's in the Ready Queue
        while (current != NULL)
        {   
            // Check if the names match and return the current PCB if they do
            if (strcmp(current->process_name, name) == 0)
            {
                return current;
            }
            current = current->next;
        }
    }
    // Search Blocked Queue if it exists
    if (blocked_q != NULL)
    {
        current = blocked_q->front;   // start with the pcb in the front

        // Loop through all PCB's in the Blocked Queue
        while (current != NULL)
        {   
            // Check if the names match and return the current PCB if they do
            if (strcmp(current->process_name, name) == 0)
            {
                return current;
            }
            current = current->next;
        }
    }
    // Search Suspended Ready Queue if it exists
    if (susp_ready_q != NULL)
    {
        current = susp_ready_q->front;   // start with the pcb in the front

        // Loop through all PCB's in the Suspended Ready Queue
        while (current != NULL)
        {   
            // Check if the names match and return the current PCB if they do
            if (strcmp(current->process_name, name) == 0)
            {
                return current;
            }
            current = current->next;
        }
    }
    // Search Suspended Blocked Queue if it exists
    if (susp_blocked_q != NULL)
    {
        current = susp_blocked_q->front;   // start with the pcb in the front

        // Loop through all PCB's in the Suspended Blocked Queue
        while (current != NULL)
        {   
            // Check if the names match and return the current PCB if they do
            if (strcmp(current->process_name, name) == 0)
            {
                return current;
            }
            current = current->next;
        }
    }
    return NULL; // Return NULL if not found
}

// Function to insert a PCB into the appropriate queue
void pcb_insert(struct pcb *pcb)
{
    // Make sure PCB isn't null
    if (pcb == NULL)
    {
        return;
    }

    // Insert into a Non-Suspended Queue
    if (pcb->dispatching_state == NOT_SUSPENDED)
    {
        // Insert into Ready Queue
        if (pcb->execution_state == READY)
        {
            // Creates the Ready Queue if it doesn't already exist
            if (ready_q == NULL) 
            {
                ready_q = (struct queue *)sys_alloc_mem(sizeof(struct queue));
                ready_q->front = pcb;
                pcb->next = NULL;
                return;
            }
            else 
            {
                struct pcb *current = ready_q->front;   // start with the pcb in the front
                
                // Check if Ready Queue is empty (already checked that it exists)
                if (current == NULL)
                {
                    ready_q->front = pcb;
                    return;
                }

                // If new PCB has a higher priority than the head of the Ready Queue, it becomes the new head
                if (current->process_priority > pcb->process_priority)
                {
                    ready_q->front = pcb;
                    pcb->next = current;
                    return;
                }

                // Loop through all PCB's in the Ready Queue
                while (current->next != NULL)
                {   
                    // Check if the PCB after 'current' has a higher priority than the new pcb
                    // Insert new PCB if so
                    if (current->next->process_priority > pcb->process_priority)
                    {
                        pcb->next = current->next;
                        current->next = pcb;
                        return;
                    }
                    current = current->next;
                }
                // PCB inserted at the end of queue if no existing PCBs have a lower priority (larger int value = lower priority)
                current->next = pcb;
                pcb->next = NULL;
                return;
            }
        }
        // Insert into Blocked Queue
        else
        {
            // Creates the Blocked Queue if it doesn't already exist
            if (blocked_q == NULL) 
            {
                blocked_q = (struct queue *)sys_alloc_mem(sizeof(struct queue));
                blocked_q->front = pcb;
                pcb->next = NULL;
                return;
            }
            else 
            {
                struct pcb *current = blocked_q->front;   // start with the pcb in the front

                // Loop through all PCB's in the Blocked Queue until you reach the end
                while (current->next != NULL)
                {   
                    current = current->next;
                }
                // PCB inserted at the end of queue (simple FIFO ordering)
                current->next = pcb;
                pcb->next = NULL;
                return;
            }
        }
    }
    // Insert into a Suspended Queue
    else
    {
        // Insert into Suspended Ready Queue
        if (pcb->execution_state == READY)
        {
            // Creates the Suspended Ready Queue if it doesn't already exist
            if (susp_ready_q == NULL) 
            {
                susp_ready_q = (struct queue *)sys_alloc_mem(sizeof(struct queue));
                susp_ready_q->front = pcb;
                pcb->next = NULL;
                return;
            }
            else 
            {
                struct pcb *current = susp_ready_q->front;   // start with the pcb in the front

                // Check if Suspended Ready Queue is empty (already checked that it exists)
                if (current == NULL)
                {
                    susp_ready_q->front = pcb;
                    return;
                }

                // If new PCB has a higher priority than the head of the Suspended Ready Queue, it becomes the new head
                if (current->process_priority > pcb->process_priority)
                {
                    susp_ready_q->front = pcb;
                    pcb->next = current;
                    return;
                }

                // Loop through all PCB's in the Suspended Ready Queue
                while (current->next != NULL)
                {   
                    // Check if the PCB after 'current' has a higher priority than the new pcb
                    // Insert new PCB if so
                    if (current->next->process_priority > pcb->process_priority)
                    {
                        pcb->next = current->next;
                        current->next = pcb;
                        return;
                    }
                    current = current->next;
                }
                // PCB inserted at the end of queue if no existing PCBs have a lower priority (larger int value = lower priority)
                current->next = pcb;
                pcb->next = NULL;
                return;
            }
        }
        // Insert into Suspended Blocked Queue
        if (pcb->execution_state == BLOCKED)
        {
            // Creates the Suspended Blocked Queue if it doesn't already exist
            if (susp_blocked_q == NULL) 
            {
                susp_blocked_q = (struct queue *)sys_alloc_mem(sizeof(struct queue));
                susp_blocked_q->front = pcb;
                pcb->next = NULL;
                return;
            }
            else 
            {
                struct pcb *current = susp_blocked_q->front;   // start with the pcb in the front

                // Loop through all PCB's in the Suspended Blocked Queue until you reach the end
                while (current->next != NULL)
                {   
                    current = current->next;
                }
                // PCB inserted at the end of queue (simple FIFO ordering)
                current->next = pcb;
                pcb->next = NULL;
                return;
            }
        }
    }
}
// Function to remove a PCB from its current queue
int pcb_remove(struct pcb *pcb)
{
    if (pcb == NULL)
    {
        return -1; // Error: NULL pointer
    }

    // Remove from Ready Queue if it exists
    if (ready_q != NULL)
    {
        struct pcb *current = ready_q->front;
        struct pcb *prev = NULL;

        // Search for the PCB in the Ready Queue
        while (current != NULL)
        {
            if (current == pcb)
            {
                // PCB found, remove it from the queue
                if (prev != NULL)
                {
                    prev->next = current->next;
                }
                else
                {
                    ready_q->front = current->next;
                }
                pcb->next = NULL;
                return 0; // Success
            }
            prev = current;
            current = current->next;
        }
    }

    // Remove from Blocked Queue if it exists
    if (blocked_q != NULL)
    {
        struct pcb *current = blocked_q->front;
        struct pcb *prev = NULL;

        // Search for the PCB in the Blocked Queue
        while (current != NULL)
        {
            if (current == pcb)
            {
                // PCB found, remove it from the queue
                if (prev != NULL)
                {
                    prev->next = current->next;
                }
                else
                {
                    blocked_q->front = current->next;
                }
                pcb->next = NULL;
                return 0; // Success
            }
            prev = current;
            current = current->next;
        }
    }

    // Remove from Suspended Ready Queue if it exists
    if (susp_ready_q != NULL)
    {
        struct pcb *current = susp_ready_q->front;
        struct pcb *prev = NULL;

        // Search for the PCB in the Suspended Ready Queue
        while (current != NULL)
        {
            if (current == pcb)
            {
                // PCB found, remove it from the queue
                if (prev != NULL)
                {
                    prev->next = current->next;
                }
                else
                {
                    susp_ready_q->front = current->next;
                }
                pcb->next = NULL;
                return 0; // Success
            }
            prev = current;
            current = current->next;
        }
    }

    // Remove from Suspended Blocked Queue if it exists
    if (susp_blocked_q != NULL)
    {
        struct pcb *current = susp_blocked_q->front;
        struct pcb *prev = NULL;

        // Search for the PCB in the Suspended Blocked Queue
        while (current != NULL)
        {
            if (current == pcb)
            {
                // PCB found, remove it from the queue
                if (prev != NULL)
                {
                    prev->next = current->next;
                }
                else
                {
                    susp_blocked_q->front = current->next;
                }
                pcb->next = NULL;
                return 0; // Success
            }
            prev = current;
            current = current->next;
        }
    }

    return -1; // Error: PCB not found in any queue
}

// Function to set the priority of a PCB
int pcb_set_priority(const char *name, int new_priority)
{
    struct pcb *pcb = pcb_find(name);

    // Check if the PCB with the given name exists
    if (pcb == NULL)
    {
        return -1; // PCB not found
    }

    // Check if the new_priority is within the valid range (0-9)
    if (new_priority < 0 || new_priority > 9)
    {
        return -2; // Invalid priority
    }

    // If the PCB is in the ready state or the suspended ready state, adjust its position based on the new priority
    if (pcb->execution_state == READY)
    {
        // Remove the PCB from its current queue (either ready or suspended ready)
        pcb_remove(pcb);
        
        // Update the PCB's priority
        pcb->process_priority = new_priority;
        
        // Reinsert the PCB into the appropriate queue with the new priority
        pcb_insert(pcb);
    }
    else
    {
        // Update the PCB's priority without reordering in the queue
        pcb->process_priority = new_priority;
    }

    return 0; // Success
}

// getters to access the queues from the interface.c file 
struct queue* get_ready_q() {
    return ready_q;
}

struct queue* get_blocked_q() {
    return blocked_q;
}

struct queue* get_susp_ready_q() {
    return susp_ready_q;
}

struct queue* get_susp_blocked_q() {
    return susp_blocked_q;
}

