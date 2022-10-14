[bits 64]

global breakpoint
global stop_interrupts
global start_interrupts
global serial_debug
global halt


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
    mov dx,0x3f8    ;0xE9 if using Bochs, 0x3f8 if using QEMU
    mov eax,edi
    out dx,eax
    ret

halt:
    cli
    hlt

