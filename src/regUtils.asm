[bits 64]


global readCRO
global writeCR0
global readCR3
global writeCR3
global readCR2

readCRO:
    mov rax,cr0
    ret

writeCR0:
    mov cr0,rdi
    ret

readCR3:
    mov rax, cr3
    ret

writeCR3:
    mov cr3,rdi
    ret

readCR2:
    mov rax,cr2
    ret