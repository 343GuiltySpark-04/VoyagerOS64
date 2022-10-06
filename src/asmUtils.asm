[bits 64]

global breakpoint
global stop_interrupts
global start_interrupts

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
