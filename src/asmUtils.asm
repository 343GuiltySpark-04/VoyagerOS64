[bits 64]

global breakpoint
global stop_interrupts
global start_interrupts
global serial_debug
global halt
global inb
global outb
global cpuid_check_sse
global cpuid_check_xsave
global cpuid_check_pcid
global cpuid_check_pae
global cpuid_check_mce
global cpuid_check_apic
global cpuid_check_mca
global cpuid_check_acpi
global cpuid_check_ds
global cpuid_check_tm
global cpuid_check_sep
global cpuid_check_htt
global cpuid_check_rdseed
global cpuid_check_rdrand
global cpuid_check_fpu
global cpuid_check_oxsave
global cpuid_check_avx
global cpuid_check_fxsr
global float_save
global float_load
global float_bank
global task_switch_int
global rdseed_asm
global rdrand_asm

extern no_sse
extern no_xsave
extern no_oxsave
extern no_avx



rdseed_asm:
    rdseed rax
    ret

rdrand_asm:
    rdrand rax
    ret




float_save:
    xsave [float_bank]

float_load:
    xrstor [float_bank]


task_switch_int:
    int 0x30
    ret

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

cpuid_check_sse:
    mov eax, 0x1
    cpuid
    bt edx, 25
    jnc no_sse
    mov eax, 1
    ret


cpuid_check_xsave:
    mov eax, 0x1
    cpuid
    bt ecx, 26
    jnc no_xsave
    mov eax, 1
    ret



no_fxsr:
    mov eax, 0
    ret

no_pcid:
    mov eax, 0
    ret

no_pae:
    mov eax, 0
    ret

no_mce:
    mov eax, 0
    ret

no_apic:
    mov eax, 0
    ret

no_mca:
    mov eax, 0
    ret

no_acpi:
    mov eax, 0
    ret

no_ds:
    mov eax, 0
    ret

no_tm:
    mov eax, 0
    ret

no_sep:
    mov eax, 0
    ret

no_htt:
    mov eax, 0
    ret

no_rdseed:
    mov eax, 0
    ret

no_rdrand:
    mov eax, 0
    ret

no_fpu:
    mov eax, 0
    ret


cpuid_check_fxsr:
    mov eax, 0x1
    cpuid
    bt edx, 24
    jnc no_fxsr
    mov eax, 1
    ret


cpuid_check_oxsave:
    mov eax, 0x1
    cpuid
    bt ecx, 27
    jnc no_oxsave
    mov eax, 1
    ret

cpuid_check_avx:
    mov eax, 0x1
    cpuid
    bt ecx, 28
    jnc no_avx
    mov eax, 1
    ret

cpuid_check_fpu:
    mov eax, 0x1
    cpuid
    bt edx, 0
    jnc no_fpu
    mov eax, 1
    ret

cpuid_check_rdrand:
    mov eax, 0x1
    cpuid
    bt ecx, 30  ; check the 30th bit of ECX register 
    jnc no_rdrand ; if the result is not set (ie. 0) jump to no_rdrand label 
    mov eax, 1   ; otherwise set EAX to 1 and return 
    ret 


cpuid_check_rdseed:
    mov eax, 0x7
    cpuid
    bt ebx, 18
    jnc no_rdseed
    mov eax, 1
    ret

cpuid_check_htt:
    mov eax, 0x1
    cpuid
    bt edx, 28  ; Check bit 28 of edx register
    jnc no_htt  ; Jump to no_htt if the bit is not set (carry flag is clear)
    mov eax, 1
    ret

cpuid_check_pcid:
    mov eax, 0x1
    cpuid
    bt ecx, 17
    jnc no_pcid  
    mov eax, 1
    ret
    

cpuid_check_pae:
    mov eax, 0x1
    cpuid
    bt edx, 6
    jnc no_pae
    mov eax, 1
    ret

cpuid_check_mce:
    mov eax, 0x1
    cpuid
    bt edx, 7
    jnc no_mce
    mov eax, 1
    ret
    

cpuid_check_apic:
    mov eax, 0x1
    cpuid
    test edx, 1 << 9
    jz no_apic
    mov eax, 1
    ret


cpuid_check_mca:
    mov eax, 0x1
    cpuid
    test edx, 1 << 14
    jz no_mca
    mov eax, 1
    ret

cpuid_check_acpi:
    mov eax, 0x1
    cpuid   
    test edx, 1 << 22
    jz no_acpi
    mov eax, 1
    ret

cpuid_check_ds:
    mov eax, 0x1
    cpuid   
    test edx, 1 << 21
    jz no_ds
    mov eax, 1
    ret

cpuid_check_tm:
    mov eax, 0x1
    cpuid
    test edx, 1 << 29
    jz no_tm
    mov eax, 1
    ret

cpuid_check_sep:
    mov eax, 0x1
    cpuid
    test edx, 1 << 11
    jz no_sep
    mov eax, 1
    ret

align 16
float_bank: TIMES 512 db 0