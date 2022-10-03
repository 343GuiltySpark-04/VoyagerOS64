[bits 64]

global breakpoint

; Bochs Magic Breakpoints
breakpoint:
    xchg bx,bx
    ret