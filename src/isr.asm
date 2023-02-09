extern isr_exception_handler
extern irq_handler
extern float_bank
global dyn_isr_handler

%macro isr_err_stub 1
isr_stub_%+%1:
    cli
    push %1
    jmp isr_xframe_assembler
%endmacro

%macro isr_no_err_stub 1
isr_stub_%+%1:
    cli
    push 0
    push %1
    jmp isr_xframe_assembler
%endmacro

%macro isr_irq_stub 1
isr_stub_%+%1:
    cli
    push 0
    push %1
    jmp isr_irq_xframe_assembler
%endmacro

%macro pushagrd 0
;xsave [float_bank]
push rax
push rbx
push rcx
push rdx
push rsi
push rdi
%endmacro

%macro popagrd 0
;xrstor [float_bank]
pop rdi
pop rsi
pop rdx
pop rcx
pop rbx
pop rax
%endmacro

%macro pushacrd 0
mov rax, cr0
push rax
mov rax, cr2
push rax
mov rax, cr3
push rax
mov rax, cr4
push rax
%endmacro

%macro popacrd 0
pop rax
mov cr4, rax
pop rax
mov cr3, rax
pop rax
mov cr2, rax
pop rax
mov cr0, rax
%endmacro


dyn_isr_handler:
    call rdi
    ret

isr_xframe_assembler:
    push rbp
    mov rbp, rsp
    pushagrd
    pushacrd
    mov ax, ds
    push rax
    push qword 0
    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov ss, ax

    lea rdi, [rsp + 0x10]
    call isr_exception_handler

    pop rax
    pop rax
    mov ds, ax
    mov es, ax
    popacrd
    popagrd
    pop rbp
    add rsp, 0x10
    sti
    iretq


isr_irq_xframe_assembler:
    push rbp
    mov rbp, rsp
    pushagrd
    pushacrd
    mov ax, ds
    push rax
    push qword 0
    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov ss, ax

    lea rdi, [rsp + 0x10]
    call irq_handler

    pop rax
    pop rax
    mov ds, ax
    mov es, ax
    popacrd
    popagrd
    pop rbp
    add rsp, 0x10
    sti
    iretq

isr_no_err_stub 0
isr_no_err_stub 1
isr_no_err_stub 2
isr_no_err_stub 3
isr_no_err_stub 4
isr_no_err_stub 5
isr_no_err_stub 6
isr_no_err_stub 7
isr_err_stub    8
isr_no_err_stub 9
isr_err_stub    10
isr_err_stub    11
isr_err_stub    12
isr_err_stub    13
isr_err_stub    14
isr_no_err_stub 15
isr_no_err_stub 16
isr_err_stub    17
isr_no_err_stub 18
isr_no_err_stub 19
isr_no_err_stub 20
isr_no_err_stub 21
isr_no_err_stub 22
isr_no_err_stub 23
isr_no_err_stub 24
isr_no_err_stub 25
isr_no_err_stub 26
isr_no_err_stub 27
isr_no_err_stub 28
isr_no_err_stub 29
isr_err_stub    30
isr_no_err_stub 31
isr_irq_stub 32
isr_irq_stub 33
isr_irq_stub 34
isr_irq_stub 35
isr_irq_stub 36
isr_irq_stub 37
isr_irq_stub 38
isr_irq_stub 39
isr_irq_stub 40
isr_irq_stub 41
isr_irq_stub 42
isr_irq_stub 43
isr_irq_stub 44
isr_irq_stub 45
isr_irq_stub 46
isr_irq_stub 47
isr_irq_stub 48 ; yield switch (used to trigger the software task switching code
isr_irq_stub 49
isr_irq_stub 50 ; High Priority Yield
isr_irq_stub 51
isr_irq_stub 52
isr_irq_stub 53
isr_irq_stub 54
isr_irq_stub 55
isr_irq_stub 56
isr_irq_stub 57
isr_irq_stub 58
isr_irq_stub 59
isr_irq_stub 60
isr_irq_stub 61
isr_irq_stub 62
isr_irq_stub 63
isr_irq_stub 64
isr_irq_stub 65
isr_irq_stub 66
isr_irq_stub 67
isr_irq_stub 68
isr_irq_stub 69
isr_irq_stub 70
isr_irq_stub 71
isr_irq_stub 72
isr_irq_stub 73
isr_irq_stub 74
isr_irq_stub 75
isr_irq_stub 76
isr_irq_stub 77
isr_irq_stub 78
isr_irq_stub 79
isr_irq_stub 80
isr_irq_stub 81
isr_irq_stub 82
isr_irq_stub 83
isr_irq_stub 84
isr_irq_stub 85 
isr_irq_stub 86
isr_irq_stub 87
isr_irq_stub 88
isr_irq_stub 89
isr_irq_stub 90
isr_irq_stub 91
isr_irq_stub 92
isr_irq_stub 93
isr_irq_stub 94
isr_irq_stub 95
isr_irq_stub 96
isr_irq_stub 97
isr_irq_stub 98
isr_irq_stub 99
isr_irq_stub 100

global isr_stub_table
isr_stub_table:
%assign i 0 
%rep    100
    dq isr_stub_%+i
%assign i i+1 
%endrep

global proc_yield

global high_yield_int

proc_yield:
    int 0x31
    ret

high_yield_int:
    int 0x32
    ret