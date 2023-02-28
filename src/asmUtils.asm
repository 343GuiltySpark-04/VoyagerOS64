[bits 64]

global breakpoint
global stop_interrupts
global start_interrupts
global serial_debug
global halt
global inb
global outb
global float_save
global float_load
global float_bank
global task_switch_int
global cfg_XCR0
global read_XCR0



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


align 16
float_bank: TIMES 512 db 0