// Header file for interface.c

#ifndef INTERFACE_H
#define INTERFACE_H

// Function prototype for comhand
void comhand(void);

struct pcb *load(const char *name, int process_class, int priority, void (*proc)());

#endif  // INTERFACE_H
