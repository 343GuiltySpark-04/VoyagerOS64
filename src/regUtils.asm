[bits 64]


global readCRO
global writeCR0
global readCR3
global writeCR3
global readCR2
global readRSP
global readCR4
global writeCR4
global get_apic_base_address
global cfg_XCR0
global test_em
global read_XCR0
global get_xsave_size
global readRIP



get_apic_base_address:
    mov ecx, 0x1b
    rdmsr
    and eax, 0x001FFFFF  ; extract the APIC base address from the MSR value
    shr ebx, 12         ; isolate the APIC ID from the MSR value
    lea eax, [eax + ebx] ; combine the APIC base and ID 
    ret







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

; Temporary Stage-2 diagnostic handoff.
; Raw COM1 markers deliberately avoid printf/terminal/data dependencies:
;   A = entered writeCR3 before switching
;   B = fetched/executed immediately after mov cr3
;   C = successfully read the return address from the current stack
; If all three appear, the new tables can execute this text and read RSP.
writeCR3:
    mov dx, 0x3f8
    mov al, 'A'
    out dx, al

    mov cr3, rdi

    mov al, 'B'
    out dx, al

    ; Preserve the return address in a caller-saved scratch register. Do NOT
    ; store it in RAX and then write AL: AL is the low byte of RAX and would
    ; corrupt the return address before the jump.
    pop r11
    mov al, 'C'
    out dx, al
    jmp r11

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
    mov cr4,rdi
    ret

readRIP:
    lea rax, [rel $]
    ret