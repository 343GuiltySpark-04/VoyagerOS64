global switch_to
extern current

section .text
switch_to:
    ; rdi = next process_t*
    ; Save current rsp
    mov rax, [rel current]
    test rax, rax
    jz .load_new              ; if current == 0, no save

    mov [rax + 8], rsp        ; current->rsp (offset: pid=0, rsp=8)

    ; Save callee-saved regs
    push rbp
    push rbx
    push r12
    push r13
    push r14
    push r15
    mov [rax + 8], rsp        ; update rsp again after pushes

.load_new:
    ; Load new process into current
    mov [rel current], rdi

    ; Load new rsp
    mov rsp, [rdi + 8]

    ; Restore callee-saved regs
    pop r15
    pop r14
    pop r13
    pop r12
    pop rbx
    pop rbp

    ret
