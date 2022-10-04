[bits 64]

global gdt_load

gdtr dw 0
     dq 0

gdt_load:
    mov [gdtr], RDI
    lgdt [gdtr]
    ret