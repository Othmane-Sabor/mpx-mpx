#include <mpx/sys_call.h>
#include <pcb.h>
#include <sys_req.h>
#include <string.h>
#include <mpx/device.h>
#include <mpx/serial.h>
#include <memory.h>
#include <mpx/interrupts.h>


struct pcb *current_process = NULL; 
struct pcb *next_process = NULL;  
struct context *initial_context = NULL;  
int insert_flag = 0;

void check_io_completion();
void process_io_request(struct iocb *iocb);
struct iocb *create_iocb(struct context *ctx, int operation) ;
// Auxiliary functions needed
struct dcb *get_dcb_for_device(device dev);
void enqueue_io_request(struct dcb *dcb, struct iocb *iocb);
struct iocb *dequeue_io_request(struct dcb *dcb);

struct context *sys_call(struct context *ctx) {
    unsigned int operation = ctx->eax;
    // device current_device;
    // char *current_buf;
    // int buf_size;
    while(get_ready_q()->front == NULL){
        check_io_completion();
        sti();
    }
    cli();


    if (operation == IDLE) {
        if (initial_context == NULL) {  
            initial_context = ctx;
        }
        // Handle IDLE 
        if (current_process != NULL) { //(something was running)
            current_process->execution_state = READY;
            current_process->stack_ptr = (unsigned char *) ctx;
            insert_flag = 1;
        }
    }
    else if (operation == EXIT) {
        // Handle EXIT
        if (current_process != NULL) {
            pcb_remove(current_process); // Remove current_process from its queue
            pcb_free(current_process); // Deallocate PCB resources
            current_process = NULL;
        }
    } else if (operation == READ || operation == WRITE) {
        //check_io_completion();
        // Process the I/O request
        struct iocb *new_iocb = create_iocb(ctx, operation);
        if (new_iocb != NULL) {
            process_io_request(new_iocb);
        }
    }else {
        ctx->eax = (uint32_t) -1;  // Unsupported operation
        return ctx;
    }
    
    if (get_ready_q() != NULL && get_ready_q()->front != NULL) {
            next_process = get_ready_q()->front;
            ctx = (struct context *) next_process->stack_ptr;
            pcb_remove(next_process); // Remove current_process from its queue
            pcb_free(next_process); // Deallocate PCB resources
            if (insert_flag == 1) {
                pcb_insert(current_process);
                insert_flag = 0;
            }
            current_process = next_process;
    }
    else { // if no process, load initial context
        ctx = initial_context;
        initial_context = NULL; // reset initial_context as it's now being used
    }
    ctx->eax = (uint32_t) 0;

    return ctx;
}

struct iocb *create_iocb(struct context *ctx,  int operation) {
    struct iocb *new_iocb = (struct iocb *)sys_alloc_mem(sizeof(struct iocb));
    if (!new_iocb) {
        return NULL; // Memory allocation failed
    }
    new_iocb->dev = (device)ctx->ebx;
    strcpy(new_iocb->pcb_name, current_process->process_name);
    new_iocb->opcode = operation;
    new_iocb->buf = (char *)ctx->ecx;
    new_iocb->buf_size = (int)ctx->edx;
    new_iocb->next = NULL;
    return new_iocb;
}

void check_io_completion() {
    struct dcb *dcbs[] = { get_dcb1(),get_dcb2(),get_dcb3(),get_dcb4() };

    for (int i = 0; i < 4; i++) {
        if (dcbs[i]->eventFlag == 1) { //check if any device is in use
            struct iocb *completed_iocb = dcbs[i]->active_iocb;

            if (completed_iocb != NULL) {
                struct pcb *process = pcb_find(completed_iocb->pcb_name);
                if (process) {
                    process->execution_state = READY;
                    struct context *current_process_ctx = (struct context *)(process->stack_ptr);
                    current_process_ctx->eax = (completed_iocb->opcode == READ) ?
                                            dcbs[i]->transferedCount : dcbs[i]->transferedCount;
                }

                dcbs[i]->active_iocb = NULL;
                dcbs[i]->eventFlag = 0;
                dcbs[i]->allocationStatus = AVAILABLE;

                sys_free_mem(completed_iocb);

                struct iocb *next_iocb = dequeue_io_request(dcbs[i]);
                if (next_iocb) {
                    process_io_request(next_iocb);
                }
            }
        }
    }
}

void process_io_request(struct iocb *iocb) {
    struct dcb *current_dcb = get_dcb_for_device(iocb->dev);

    if (current_dcb == NULL) {
        // Handle error: Device not found
        return;
    }

    if (current_dcb->allocationStatus == AVAILABLE) {
        // Device is available, process the request immediately
        current_dcb->allocationStatus = USE;
        current_dcb->active_iocb = iocb;

        if (iocb->opcode == READ) {
            serial_read(iocb->dev, iocb->buf, iocb->buf_size);
            current_dcb->transferedCount = iocb->buf_size; // Assuming full read
        } else if (iocb->opcode == WRITE) {
            serial_write(iocb->dev, iocb->buf, iocb->buf_size);
            current_dcb->transferedCount = iocb->buf_size; // Assuming full write
        }

        // Set eventFlag to signal completion, this might be set elsewhere in your actual I/O operation
        //current_dcb->eventFlag = 0;
    } else {
        // Device is busy, enqueue the request
        enqueue_io_request(current_dcb, iocb);
    }
}

struct dcb *get_dcb_for_device(device dev) {
    // Return the corresponding DCB based on the device
    switch (dev)
    {
    case COM1:
        return get_dcb1();
    case COM2:
        return get_dcb2();
    case COM3:
        return get_dcb3();
    case COM4:
        return get_dcb4();
    default:
        return NULL;
    }
}

void enqueue_io_request(struct dcb *dcb, struct iocb *iocb) {
    if (dcb->pending_req == NULL) {
        dcb->pending_req = iocb;
    } else {
        struct iocb *current = dcb->pending_req;
        while (current->next != NULL) {
            current = current->next;
        }
        current->next = iocb;
    }
    iocb->next = NULL;
}

struct iocb *dequeue_io_request(struct dcb *dcb) {
    if (dcb == NULL || dcb->pending_req == NULL) {
        return NULL; // No DCB or no queued requests
    }
    struct iocb *next_request = dcb->pending_req; // Get the first request in the queue

    // Update the queue head to the next element
    dcb->pending_req = next_request->next;

    next_request->next = NULL; // Clear the 'next' pointer of the dequeued request

    return next_request; // Return the dequeued request
}
