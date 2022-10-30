;C declaration:
;   void switch_to_task(thread_control_block *next_thread);
;
;WARNING: Caller is expected to disable IRQs before calling, and enable IRQs again after function returns
 
global switch_to_task

switch_to_task:
 
    ;Save previous task's state
 
    ;Notes:
    ;  For cdecl; EAX, ECX, and EDX are already saved by the caller and don't need to be saved again
    ;  EIP is already saved on the stack by the caller's "CALL" instruction
    ;  The task isn't able to change CR3 so it doesn't need to be saved
    ;  Segment registers are constants (while running kernel code) so they don't need to be saved
 
    push rbx
    push rsi
    push rdi
    push rbp
 
    mov rdi,[current_task_TCB]    ;edi = address of the previous task's "thread control block"
    mov [rdi+TCB.RSP],rsp         ;Save ESP for previous task's kernel stack in the thread's TCB
 
    ;Load next task's state
 
    mov rsi,[rsp+(4+1)*4]         ;esi = address of the next task's "thread control block" (parameter passed on stack)
    mov [current_task_TCB],rsi    ;Current task's TCB is the next task TCB
 
    mov rsp,[rsi+TCB.RSP]         ;Load ESP for next task's kernel stack from the thread's TCB
    mov rax,[rsi+TCB.CR3]         ;eax = address of page directory for next task
    mov rbx,[rsi+TCB.RSP0]        ;ebx = address for the top of the next task's kernel stack
    mov [TSS.RSP0],rbx            ;Adjust the ESP0 field in the TSS (used by CPU for for CPL=3 -> CPL=0 privilege level changes)
    mov rcx,cr3                   ;ecx = previous task's virtual address space
 
    cmp rax,rcx                   ;Does the virtual address space need to being changed?
    je .doneVAS                   ; no, virtual address space is the same, so don't reload it and cause TLB flushes
    mov cr3,rax                   ; yes, load the next task's virtual address space
.doneVAS:
 
    pop rbp
    pop rdi
    pop rsi
    pop rbx
 
    ret                           ;Load next task's EIP from its kernel stack