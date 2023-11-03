
#include <mpx/io.h>
#include <mpx/serial.h>
#include <sys_req.h>
#include <string.h>


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
