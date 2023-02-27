[bits 64]


global readCRO
global writeCR0
global readCR3
global writeCR3
global readCR2
global readRSP
global readCR4
global writeCR4
global cfg_XCR0
global test_em
global read_XCR0
global get_xsave_size



get_xsave_size:
    mov ecx, 0x0
    mov eax, 0xd
    cpuid
    movsx rax, ecx
    ret




cfg_XCR0:
    mov eax, 0x1
    cpuid
    bt ecx, 28
    jnc .noavx
    xor ecx, ecx
    mov rax, 0x7
    xor rdx, rdx
    xsetbv
    ret
.noavx:
    xor ecx, ecx
    mov rax, 0x3
    xor rdx, rdx
    xsetbv
    ret






read_XCR0:
    mov ecx, 0x0
    xgetbv
    ret




no_em:
    mov rax, 0
    ret

test_em:
    mov r13, cr0
    mov r13, 0 >> 2
    mov cr0, r13
    mov rax, 1
    ret

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