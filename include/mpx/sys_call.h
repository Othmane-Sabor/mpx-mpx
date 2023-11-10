#ifndef SYS_CALL_H
#define SYS_CALL_H

#include <stdint.h>

struct context {
    
    // General Purpose Registers
    // Segment Registers
    // Not automatically saved by CPU, we push these manually
    uint32_t gs;
    uint32_t fs;
    uint32_t es;
    uint32_t ds;
    uint32_t ss;
    
    uint32_t edi; // Original ESP before interrupt
    uint32_t esi;
    uint32_t esp;
    uint32_t ebp;
    uint32_t ebx;
    uint32_t edx;
    uint32_t ecx;
    uint32_t eax;
    // Automatically pushed by the CPU
    uint32_t eip;
    uint32_t cs;
    uint32_t eflags;

};

struct context* sys_call(struct context* ctx);

#endif
