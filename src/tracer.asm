; Walks backwards through the call stack and builds a list of return addresses.
; Args:
;  * Array of 64-bit addresses.
;  * Maximum number of elements in array.
; Return value: The number of addresses stored in the array.
; Calling convention: cdecl
[global walk_stack] ; Luke Stackwalker, use the Stack Luke.
walk_stack:
    ; Create stack frame & save caller's RDI and RBX.
    push rbp
    mov  rbp,       rsp
    sub  rsp,       16
    mov  [rbp - 8], rdi
    mov  [rbp - 16], rbx
    ; Set up local registers.
    xor  rax,       rax         ; RAX = return value (number of stack frames found).
    mov  rbx,       [rsp +  8]  ; RBX = old RBP.
    mov  rdi,       [rsp + 16]  ; Destination array pointer in RDI.
    mov  rcx,       [rsp + 24]  ; Maximum array size in RCX.
.walk:
    ; Walk backwards through RBP linked list, storing return addresses in RDI array.
    test rbx,       rbx
    jz   .done
    mov  rdx,       [rbx +  8]  ; RDX = previous stack frame's IP.
    mov  rbx,       [rbx +  0]  ; RBX = previous stack frame's BP.
    mov  [rdi],     rdx         ; Copy IP.
    add  rdi,       8
    inc  rax
    cmp  rcx, rax
    jae  .done
    loop .walk
.done:
    ; Restore caller's RDI and RBX, leave stack frame & return RAX.
    mov  rdi,       [rbp - 8]
    mov  rbx,       [rbp - 16]
    leave
    ret
    

[extern dump_hex]
[global stack_dump_asm]

stack_dump_asm:
    push rbp
    mov rdi, [rbp]
    mov rsi, 0x1000
    call dump_hex
    pop rbp
    leave
    hlt