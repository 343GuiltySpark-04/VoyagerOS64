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
global float_save
global float_load
global float_bank
global task_switch_int
global vendor_str1
global vendor_str2
global vendor_str3

extern no_sse
extern no_xsave


vendor_str1:
    mov eax, 0x0
    cpuid
    mov eax, ebx
    ret

vendor_str2:
    mov eax, 0x0
    cpuid
    mov eax, edx
    ret

vendor_str3:
    mov eax, 0x0
    cpuid
    mov eax, ecx
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
    test edx, 1 << 25
    jz no_sse
    mov eax, 1
    ret


cpuid_check_xsave:
    mov eax, 0x1
    cpuid
    test ecx, 1 << 26
    jz no_xsave
    mov eax, 1
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




cpuid_check_pcid:
    mov eax, 0x1
    cpuid
    test ecx, 1 << 17
    jz no_pcid  
    mov eax, 1
    ret
    

cpuid_check_pae:
    mov eax, 0x1
    cpuid
    test edx, 1 << 6
    jz no_pae
    mov eax, 1
    ret

cpuid_check_mce:
    mov eax, 0x1
    cpuid
    test edx, 1 << 7
    jz no_mce
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

align 16
float_bank: TIMES 512 db 0