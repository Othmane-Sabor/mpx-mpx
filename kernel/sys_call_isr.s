bits 32
global sys_call_isr

extern sys_call
sys_call_isr:
    push eax
    push ecx
    push edx
    push ebx
    push ebp
    push esp
    push esi
    push edi               
    push ss
    push ds
    push es
    push fs
    push gs
    push esp
    call sys_call           ; Call sys_call
    mov esp, eax            ; Set ESP based on the return value (in EAX)
    pop gs
    pop fs
    pop es
    pop ds
    pop ss
    pop edi
    pop esi
    add esp, 4
    pop ebp
    pop ebx
    pop edx
    pop ecx
    pop eax
    iret                    ; Return from ISR