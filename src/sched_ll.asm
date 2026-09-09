[bits 64]

global switch_to
extern current

section .text

; Cooperative kernel-task context switch.
; rdi = next process_t*
;
; The caller reaches this routine through an ordinary SysV function call, so
; only callee-saved registers need to survive. IRQ/preemptive switching must use
; an interrupt-frame-aware path and must not call this routine directly.
switch_to:
    mov rax, [rel current]
    test rax, rax
    jz .load_new

    ; Save the outgoing task's callee-saved context. The return address for the
    ; switch_to() call is already on its stack beneath these registers.
    push rbp
    push rbx
    push r12
    push r13
    push r14
    push r15
    mov [rax + 8], rsp        ; process_t.rsp

.load_new:
    mov [rel current], rdi
    mov rsp, [rdi + 8]        ; process_t.rsp

    pop r15
    pop r14
    pop r13
    pop r12
    pop rbx
    pop rbp
    ret
