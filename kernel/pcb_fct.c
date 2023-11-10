#include <stdlib.h>
#include <string.h>

// PCB states
#define READY 0
#define SUSPENDED 1

// PCB structure
struct pcb
{
    char process_name[256];
    int process_class;
    int process_priority;
    int state;
    // Add other PCB fields as needed
    struct pcb *next; // Pointer for queue management
};

// PCB queue structure
struct pcb_queue
{
    struct pcb *front;
    struct pcb *rear;
};

// Function to allocate memory for a new PCB
struct pcb *allocate(void)
{
    struct pcb *new_pcb = (struct pcb *)malloc(sizeof(struct pcb));
    if (new_pcb != NULL)
    {
        // Initialize PCB fields here
        memset(new_pcb->process_name, 0, sizeof(new_pcb->process_name));
        new_pcb->process_class = 0;
        new_pcb->process_priority = 0;
        new_pcb->state = READY; // Default state is Ready
        new_pcb->next = NULL;
        // Perform other initializations as needed
    }
    return new_pcb;
}

// Function to free memory associated with a PCB
int pcb_free(struct pcb *pcb)
{
    if (pcb == NULL)
    {
        return -1; // Error: NULL pointer
    }

    free(pcb);
    return 0; // Success
}

// Function to allocate and initialize a new PCB
struct pcb *pcb_setup(const char *name, int process_class, int priority)
{
    struct pcb *new_pcb = allocate();
    if (new_pcb != NULL)
    {
        strncpy(new_pcb->process_name, name, sizeof(new_pcb->process_name));
        new_pcb->process_class = process_class;
        new_pcb->process_priority = priority;
        new_pcb->state = READY; // Set the state to Ready
        // Perform other initializations as needed
    }
    return new_pcb;
}

// Function to find a PCB by name
struct pcb *pcb_find(const char *name, struct pcb_queue *queue)
{
    if (queue == NULL)
    {
        return NULL; // Error: Invalid queue
    }

    struct pcb *current = queue->front;

    while (current != NULL)
    {
        if (strcmp(current->process_name, name) == 0)
        {
            return current; // Found the PCB
        }
        current = current->next;
    }

    return NULL; // Return NULL if not found
}

// Function to insert a PCB into the appropriate queue
void pcb_insert(struct pcb *pcb, struct pcb_queue *queue)
{
    if (pcb == NULL || queue == NULL)
    {
        return; // Error: Invalid arguments
    }

    // Implement the insertion logic based on state and priority
    // Assuming a priority queue (higher priority at the front)
    if (queue->front == NULL || pcb->process_priority > queue->front->process_priority)
    {
        pcb->next = queue->front;
        queue->front = pcb;
    }
    else
    {
        struct pcb *current = queue->front;
        while (current->next != NULL && pcb->process_priority <= current->next->process_priority)
        {
            current = current->next;
        }
        pcb->next = current->next;
        current->next = pcb;
    }

    if (queue->rear == NULL)
    {
        queue->rear = pcb;
    }
}

// Function to remove a PCB from its current queue
int pcb_remove(struct pcb *pcb, struct pcb_queue *queue)
{
    if (pcb == NULL || queue == NULL)
    {
        return -1; // Error: Invalid arguments
    }

    struct pcb *current = queue->front;
    struct pcb *previous = NULL;

    while (current != NULL && current != pcb)
    {
        previous = current;
        current = current->next;
    }

    if (current == NULL)
    {
        return -1; // Error: PCB not found in the queue
    }

    if (previous == NULL)
    {
        // PCB is at the front of the queue
        queue->front = current->next;
        if (queue->rear == current)
        {
            queue->rear = NULL;
        }
    }
    else
    {
        previous->next = current->next;
        if (queue->rear == current)
        {
            queue->rear = previous;
        }
    }

    pcb->next = NULL; // Disconnect PCB from the queue
    return 0;         // Success
}
