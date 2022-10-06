[bits 64]

global breakpoint
global stop_interrupts
global start_interrupts
global serial_debug


; Bochs Magic Breakpoints
breakpoint:
    xchg bx,bx
    ret


stop_interrupts:
    cli
    ret

start_interrupts:
    sti 
    ret

serial_debug:
    mov dx,0xe9
    mov eax,edi
    out dx,eax
    ret

