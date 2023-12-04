bits 32
global isr

extern serial_interrupt
isr :
    call serial_interrupt
    iret
