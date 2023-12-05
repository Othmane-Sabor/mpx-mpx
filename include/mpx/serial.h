#ifndef MPX_SERIAL_H
#define MPX_SERIAL_H

#include <stddef.h>
#include <mpx/device.h>
#include <sys_req.h>

/**
 @file mpx/serial.h
 @brief Kernel functions and constants for handling serial I/O
*/

/**
 Initializes devices for user input and output
 @param device A serial port to initialize (COM1, COM2, COM3, or COM4)
 @return 0 on success, non-zero on failure
*/
int serial_init(device dev);

/**
 Writes a buffer to a serial port
 @param device The serial port to output to
 @param buffer A pointer to an array of characters to output
 @param len The number of bytes to write
 @return The number of bytes written
*/
int serial_out(device dev, const char *buffer, size_t len);

/**
 Reads a string from a serial port
 @param device The serial port to read data from
 @param buffer A buffer to write data into as it is read from the serial port
 @param count The maximum number of bytes to read
 @return The number of bytes read on success, a negative number on failure
*/   		   

int serial_poll(device dev, char *buffer, size_t len);

int serial_open(device dev, int speed);
int serial_read(device dev, char *buf, size_t len);
int serial_write(device dev, char *buf, size_t len);
extern void isr(void*);

typedef enum { NOT_USE, IN_USE } AllocationStatus;

#define MAX_BUFFER_SIZE 50
#define AVAILABLE 1
#define USE 0
// Device Control Block (DCB) for Serial Port Driver
struct dcb{
    device dev;
    // available or in use by process X
    AllocationStatus allocationStatus; // Device allocation status
    int isOpen;              // Flag indicating whether the port is open (0 - closed, 1 - open)
    int eventFlag;           // Set to 0 at the beginning of an operation, 1 when the operation is complete
    op_code statusCode; // write read idle
    // Addresses and counters associated with the current input buffer
    char* inputBufferAddress;
    int inputBufferCounter;
    int transferedCount;
    // // Addresses and counters associated with the current output buffer
    // char* outputBufferAddress;
    // int outputBufferCounter;
    // int writtenCount;

    // Input ring buffer and associated properties
    char inputRingBuffer[MAX_BUFFER_SIZE];
    int inputRingBufferInputIndex;
    int inputRingBufferCounter;

    // queue
    struct iocb* active_iocb;
    struct iocb* pending_req;
};

struct iocb{
    device dev;
    char pcb_name[8];
    int opcode;
    char *buf;
    int buf_size;
    struct iocb* next;
};


// Getter function declarations
struct dcb *get_dcb1();
struct dcb *get_dcb2();
struct dcb *get_dcb3();
struct dcb *get_dcb4();


static struct dcb *dcb1;
static struct dcb *dcb2;
static struct dcb *dcb3;
static struct dcb *dcb4;


#endif

