
#include <mpx/io.h>
#include <mpx/serial.h>
#include <sys_req.h>
#include <string.h>


#include <memory.h>
#include <mpx/interrupts.h>




enum uart_registers {
	RBR = 0,	// Receive Buffer
	THR = 0,	// Transmitter Holding
	DLL = 0,	// Divisor Latch LSB
	IER = 1,	// Interrupt Enable
	DLM = 1,	// Divisor Latch MSB
	IIR = 2,	// Interrupt Identification
	FCR = 2,	// FIFO Control
	LCR = 3,	// Line Control
	MCR = 4,	// Modem Control
	LSR = 5,	// Line Status
	MSR = 6,	// Modem Status
	SCR = 7,	// Scratch
};


static int initialized[4] = { 0 };

struct dcb *dcb1;
struct dcb *dcb2;
struct dcb *dcb3;
struct dcb *dcb4;

struct dcb *get_dcb1() {
    return dcb1;
}

struct dcb *get_dcb2() {
    return dcb2;
}

struct dcb *get_dcb3() {
    return dcb3;
}

struct dcb *get_dcb4() {
    return dcb4;
}
void set_dcb1(struct dcb *value) {
    dcb1 = value;
}

void set_dcb2(struct dcb *value) {
    dcb2 = value;
}

void set_dcb3(struct dcb *value) {
    dcb3 = value;
}

void set_dcb4(struct dcb *value) {
    dcb4 = value;
}


void serial_interrupt(void){

}

int serial_open(device dev, int speed){
    struct dcb* current_dcb;
    long brd;
    int vector;
    int pic_lvl;

    switch (dev){
        case COM1:
            // add dev
            dcb1 = (struct dcb *) sys_alloc_mem(sizeof(struct dcb));
        
            current_dcb = dcb1;
            vector = 0x24;
            pic_lvl= 4;
            break;
        case COM2:
            dcb2 = (struct dcb *) sys_alloc_mem(sizeof(struct dcb));
            
            current_dcb = dcb2;
            vector = 0x23;
            pic_lvl= 3;

            break;
        case COM3:
            dcb3 = (struct dcb *) sys_alloc_mem(sizeof(struct dcb));
            
            current_dcb = dcb3;
            vector = 0x24;
            pic_lvl = 4;
            break;
        case COM4:
            dcb4 = (struct dcb *) sys_alloc_mem(sizeof(struct dcb));
            
            current_dcb = dcb4;
            vector = 0x23;
            pic_lvl = 3;
            break;
        default:
            return -101; // invalid
    }

    if(speed!= 110 && speed!=150 && speed!=300 && speed!=300 && speed!=600 && speed!=1200 && speed!=2400 && speed!=4800 && speed!=9600 && speed!=19200){
        return -102;  // Invalid baud rate divisor
    }else{
        if(speed >0){
            brd = 115200 / (long) speed;
        }
        else{
            return -102;
        }
    }

    //if ( current_dcb -> port_flag != 0){
    if (current_dcb -> isOpen == 1){
        return -103;  // already open
    }else{
        current_dcb -> statusCode = IDLE;
        current_dcb -> eventFlag = AVAILABLE; // NOT SURE
        current_dcb -> isOpen = 1; 
        current_dcb -> inputRingBufferInputIndex = 0;
        current_dcb ->inputRingBufferCounter = 0; 
        current_dcb -> inputBufferCounter = 0;
        current_dcb -> transferedCount = 0;

        idt_install(vector, isr);
        outb(dev + LCR, 0x80); //set line control register
        outb(dev + DLL,(unsigned char) brd & 0x00FF);	//set bsd least sig bit
        outb(dev + DLM, (unsigned char) ((brd & 0xFF00)>>8));	//brd most significant bit
        outb(dev + LCR, 0x03);	//lock divisor; 8bits, no parity, one stop

        cli();                          // Disable interrupts
        int mask = inb(0x21);           // Read current mask
        mask &= ~(1 << pic_lvl);        // Clear bit of pic_lvl (for IRQ pic_lvl)
        outb(0x21, mask);               // Write back the new mask
        sti();                          // Re-enable interrupts

        outb(dev + MCR, 0x08);	//enable interrupts, rts/dsr set
        outb(dev + IER, 0x01);	//enable interrupts
        (void) inb(dev);

        return 0;
    }

}

int serial_close(device dev){
    struct dcb *current_dcb;
    int pic_lvl;

    switch (dev)
    {
    case COM1:
        current_dcb = dcb1;
        pic_lvl = 4;
        break;
    case COM2:
        current_dcb = dcb2;
        pic_lvl = 3;
        break;
    case COM3:
        current_dcb = dcb3;
        pic_lvl = 4;
        break;
    case COM4:
        current_dcb = dcb4;
        pic_lvl = 3;
        break;

    default:
        return -202; //invalid dev
    }

    //if (current_dcb -> port_flag !=0) 
    if( current_dcb -> isOpen != 0)
        return -201; //port not open
    else{
        //Disable the appropriate level in the PIC mask register
        current_dcb -> isOpen = 0;
        cli ();
        int mask = inb (0x21);
        mask |= (1 << pic_lvl); // Mask (disable) hardware IRQ 
        //mask = mask | pic_lvl;
        outb (0x21, mask);
        sti ();

        outb(dev + MSR , 0x00); //load zero to modem status register
        outb(dev + IER, 0x00); //load zero to interrupt enable register
    
        return 0;
    }
}

int serial_write(device dev, char *buf, size_t len){
    struct dcb *current_dcb;
    switch (dev)
    {
    case COM1:
        current_dcb = dcb1;
        break;
    case COM2:
        current_dcb = dcb2;
        break;
    case COM3:
        current_dcb = dcb3;
        break;
    case COM4:
        current_dcb = dcb4;
        break;
    default:
        return 402; //invalid com
    }

    if (buf == NULL){
        return -402; // Invalid address
    }
    else if(len<=0){
        return -403; // Invalid size
    }else {
        if( current_dcb -> isOpen ==1 && current_dcb->statusCode == IDLE){
                //serial_open(dev,19200);

                if(current_dcb->eventFlag == USE){
                    return -404; // busy 
                }else{
                    current_dcb->statusCode = WRITE;
                    current_dcb->eventFlag = USE; 
                    current_dcb->inputBufferAddress = buf;
                    current_dcb->inputBufferCounter = (int) len;
                    //perform step 5 of serial_read
                    outb(dev,buf[0]);
                    //current_dcb->eventFlag = AVAILABLE; 
                    outb(dev + IER, inb(dev+IER)|0x02);
                }
            }else{
                if(current_dcb->isOpen ==0)
                    return -401; // not open
                else   
                    return -404;
            }
    }
    return 0;
}


int serial_read(device dev, char *buf, size_t len) {
    // Step 1: Validate parameters
    if (buf == NULL || len == 0) {
        return -303; // Invalid buffer address or count value
    }

    // Obtain the correct DCB based on the device
    struct dcb *current_dcb;
    switch (dev) {
        case COM1:
            current_dcb = dcb1;
            break;
        case COM2:
            current_dcb = dcb2;
            break;
        case COM3:
            current_dcb = dcb3;
            break;
        case COM4:
            current_dcb = dcb4;
            break;
        default:
            return -301; // Port not open
    }

    // Step 2: Ensure that the port is open and the status is idle
    if (current_dcb->isOpen == 0 || current_dcb->statusCode != IDLE) {
        return -301; // Port not open
    }

    // Step 3: Initialize input buffer variables and set status to reading
    current_dcb->statusCode = READ;
    current_dcb->inputBufferAddress =buf;
    current_dcb->inputBufferCounter = 0;
    current_dcb->transferedCount = 0;
    // Step 4: Clear caller’s event flag
    current_dcb->eventFlag = 0;

    // Step 5: Copy characters from ring buffer to requestor’s buffer
    cli(); // Disable interrupts during copying
    while (current_dcb->inputRingBufferCounter > 0 && current_dcb->transferedCount < (int) len) {
        char received_char = current_dcb->inputRingBuffer[current_dcb->inputRingBufferInputIndex++];
        current_dcb->inputBufferAddress[current_dcb->inputBufferCounter++] = received_char;
        current_dcb->transferedCount++;

        // Handle ring buffer overflow by resetting the index
        if (current_dcb->inputRingBufferInputIndex >= MAX_BUFFER_SIZE) {
            current_dcb->inputRingBufferInputIndex = 0;
        }

        // If newline is received, terminate reading
        if (received_char == '\n') {
            break;
        }
    }
    sti(); // Re-enable interrupts after copying

    // Step 6: If more characters are needed, return
    if (current_dcb->transferedCount < (int) len) {
        return 0;
    }

    // Step 7: Reset DCB status to idle, set event flag, and return actual count
    current_dcb->statusCode = IDLE;
    current_dcb->eventFlag = 1;
    return current_dcb->transferedCount;
}


static int serial_devno(device dev)
{
	switch (dev) {
	case COM1: return 0;
	case COM2: return 1;
	case COM3: return 2;
	case COM4: return 3;
	}
	return -1;
}

int serial_init(device dev)
{

	int dno = serial_devno(dev);
	if (dno == -1) {
		return -1;
	}
	outb(dev + IER, 0x00);	//disable interrupts
	outb(dev + LCR, 0x80);	//set line control register
	outb(dev + DLL, 115200 / 9600);	//set bsd least sig bit
	outb(dev + DLM, 0x00);	//brd most significant bit
	outb(dev + LCR, 0x03);	//lock divisor; 8bits, no parity, one stop
	outb(dev + FCR, 0xC7);	//enable fifo, clear, 14byte threshold
	outb(dev + MCR, 0x0B);	//enable interrupts, rts/dsr set
	(void)inb(dev);		//read bit to reset port
	initialized[dno] = 1;
	return 0;
}

int serial_out(device dev, const char *buffer, size_t len)
{
	int dno = serial_devno(dev);
	if (dno == -1 || initialized[dno] == 0) {
		return -1;
	}
	for (size_t i = 0; i < len; i++) {
		outb(dev, buffer[i]);
	}
	return (int)len;
}


// Helper function to redraw characters from a position
void redraw_from_position(device dev, char *buffer, size_t start, size_t end) {
    for (size_t i = start; i < end; i++) {
        serial_out(dev, &buffer[i], 1);
    }
}

// Helper function to move cursor left by a certain amount
void move_cursor_left(device dev, size_t amount) {
    for (size_t i = 0; i < amount; i++) {
        serial_out(dev, "\x1B\x5B\x44", 3);  // ANSI sequence for moving cursor left
    }
}

int serial_poll(device dev, char *buffer, size_t len) {
    size_t bytesRead = 0;
    size_t cursorPosition = 0;  // Current position of the cursor
    char ch;

    while (bytesRead < len - 1) {  // -1 to leave space for null terminator
        while (!(inb(dev + LSR) & 0x01));  // Wait until data is available         
                                // LSR indicates data is available

        ch = inb(dev);

        // Handle escape sequences (arrow keys are escape sequences)
        if (ch == '\x1B') {
            char next1 = inb(dev);
            char next2 = inb(dev);

            if (next1 == '\x5B') {
                switch (next2) {

                    case '\x41':  // Up arrow
                        // handle the Up arrow key here
                        break;
                    case '\x42':  // Down arrow
                        // handle the Down arrow key here
                        break;
                    case '\x43':  // Right arrow
                        // Move cursor to the right if it is not at the end
                        if (cursorPosition < bytesRead) {
                            serial_out(dev, "\x1B\x5B\x43", 3);
                            cursorPosition++;
                        }
                        break;
                    case '\x44':  // Left arrow
                        // Move cursor to the left only if not at the start of the input
                        if (cursorPosition > 0) {
                            serial_out(dev, "\x1B\x5B\x44", 3);
                            cursorPosition--;
                        }
                        break;
                    default:
                        break;
                }
                continue;  // Skip the rest of the loop for this iteration
            }
        }

        switch (ch) {
            case '\r':  // Carriage return
                ch = '\n';  // Convert carriage return to newline
                buffer[bytesRead++] = ch;
                serial_out(dev, &ch, 1);  // Echo to user
                break;

            case 0x7F:  // Backspace ascii for backspace
            if (cursorPosition > 0) {
                // Shift characters to the left starting from the cursor position
                for (size_t i = cursorPosition - 1; i < bytesRead - 1; i++) {
                    buffer[i] = buffer[i + 1];
                }
                bytesRead--;
                cursorPosition--;

                // Move the cursor left
                move_cursor_left(dev, 1);

                // Redraw from the current cursor position to the end of the buffer
                redraw_from_position(dev, buffer, cursorPosition, bytesRead);

                // Clear the character at the new end of the buffer
                serial_out(dev, " ", 1);

                // Move the cursor back to its original position after clearing the character
                move_cursor_left(dev, bytesRead - cursorPosition + 1 );
            }
            break;

        case 0x7E:  // ASCII for delete 
            if (cursorPosition < bytesRead) {
                for (size_t i = cursorPosition; i < bytesRead - 1; i++) {
                    buffer[i] = buffer[i + 1];
                }
                bytesRead--;

                redraw_from_position(dev, buffer, cursorPosition, bytesRead);
                serial_out(dev, " ", 1);  // Clear the last character
                move_cursor_left(dev, bytesRead - cursorPosition + 1);
            }
            break;


            // Add cases for arrow keys if needed (they often send multi-byte sequences)

            default:
                // Handle regular characters
                if (ch >= ' ' && ch <= '~') {
                    for (size_t i = bytesRead; i > cursorPosition; i--) {
                        buffer[i] = buffer[i - 1];
                    }
                    buffer[cursorPosition] = ch;
                    bytesRead++;
                    cursorPosition++;
                    // serial_out(dev, &ch, 1);  // Echo to user
                    redraw_from_position(dev, buffer, cursorPosition - 1, bytesRead);
                    move_cursor_left(dev, bytesRead - cursorPosition);  // Move cursor back
        
                }
                break;
        }

        if (ch == '\n') {
            break;  // Break on newline for command processing
        }
    }

    buffer[bytesRead] = '\0';  // Null-terminate the string
    return bytesRead;
}
