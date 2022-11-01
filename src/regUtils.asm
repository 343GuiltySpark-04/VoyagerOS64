[bits 64]


global readCRO
global writeCR0
global readCR3
global writeCR3
global readCR2
global readRSP
global readCR4
global writeCR4

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

readRSP:
    mov rax,rsp
    ret

readCR4:
    mov rax, cr4
    ret

writeCR4:
    mov cr4, rdi
    ret