[bits 64]

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
global cpuid_check_vmx
global cpuid_check_htt
global cpuid_check_fpu
global cpuid_check_msr

extern no_sse
extern no_xsave

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

no_vmx:
    mov eax, 0
    ret
no_htt:
    mov eax, 0
    ret

no_fpu:
    mov eax, 0
    ret

no_msr
    mov eax, 0
    ret


cpuid_check_msr:
    mov eax, 0x1
    cpuid
    test edx, 1 << 5
    jz no_msr
    mov eax, 1
    ret


cpuid_check_fpu:
    mov eax, 0x1
    cpuid
    test edx, 1 << 0
    jz no_fpu
    mov eax, 1
    ret

cpuid_check_htt:
    mov eax, 0x1
    cpuid
    test edx, 1 << 28
    jz no_htt
    mov eax, 1
    ret

cpuid_check_vmx:
    mov eax, 0x1
    cpuid
    test ecx, 1 << 5
    jz no_vmx
    mov eax, 1
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

