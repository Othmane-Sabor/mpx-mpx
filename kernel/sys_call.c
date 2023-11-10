#include <mpx/sys_call.h>
#include <pcb.h>
#include <sys_req.h>
#include <string.h>

struct PCB *current_process = NULL;  
 struct context *initial_context = NULL;  


struct context *sys_call(struct context *ctx) {


    unsigned int operation = ctx->eax;
    if (operation == IDLE) {

        if (initial_context == NULL) {  
        initial_context = ctx;
    }

        // Handle IDLE operation
        // Update: current_process->ctx.esp = ctx->esp;
        // Add current_process back to the queue
        // Load next process context
        // Set return value in ctx->eax to 0
        ctx->eax = (uint32_t) 0;
    } else if (operation == EXIT) {
        // Handle EXIT operation
        // Delete current_process
        // Load next process context or initial_context if no other process
        // Set return value in ctx->eax to 0
        ctx->eax = (uint32_t) 0;
    } else {
        ctx->eax = (uint32_t) -1;  // Unsupported operation
    }

    return ctx;
}
