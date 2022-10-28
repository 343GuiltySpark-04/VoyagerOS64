[bits 64]

section .bss
system_timer_fractions:  resd 1          ; Fractions of 1 ms since timer initialized
system_timer_ms:         resd 1          ; Number of whole ms since timer initialized
IRQ0_fractions:          resd 1          ; Fractions of 1 ms between IRQs
IRQ0_ms:                 resd 1          ; Number of whole ms between IRQs
IRQ0_frequency:          resd 1          ; Actual frequency of PIT
PIT_reload_value:        resw 1          ; Current PIT reload value
section .text

extern config_PIT

%macro pushagrd 0
push rax
push rbx
push rcx
push rdx
push rsi
push rdi
%endmacro

%macro popagrd 0
pop rdi
pop rsi
pop rdx
pop rcx
pop rbx
pop rax
%endmacro



 ;Input
 ; rdi   Desired PIT frequency in Hz
 
 config_PIT:
    pushagrd
 
    ; Do some checking
 
    mov rax,0x10000                   ;rax = reload value for slowest possible frequency (65536)
    cmp rdi,18                        ;Is the requested frequency too low?
    jbe .gotReloadValue               ; yes, use slowest possible frequency
 
    mov rax,1                         ;ax = reload value for fastest possible frequency (1)
    cmp rdi,1193181                   ;Is the requested frequency too high?
    jae .gotReloadValue               ; yes, use fastest possible frequency
 
    ; Calculate the reload value
 
    mov rax,3579545
    mov rdx,0                         ;rdx:rax = 3579545
    div rdi                           ;rax = 3579545 / frequency, rdx = remainder
    cmp rdx,3579545 / 2               ;Is the remainder more than half?
    jb .l1                            ; no, round down
    inc rax                           ; yes, round up
 .l1:
    mov rdi,3
    mov rdx,0                         ;rdx:rax = 3579545 * 256 / frequency
    div rdi                           ;rax = (3579545 * 256 / 3 * 256) / frequency
    cmp rdx,3 / 2                     ;Is the remainder more than half?
    jb .l2                            ; no, round down
    inc rax                           ; yes, round up
 .l2:
 
 
 ; Store the reload value and calculate the actual frequency
 
 .gotReloadValue:
    push rax                          ;Store reload_value for later
    mov [PIT_reload_value],ax         ;Store the reload value for later
    mov rdi,rax                       ;rdi = reload value
 
    mov rax,3579545
    mov rdx,0                         ;rdx:rax = 3579545
    div rdi                           ;rax = 3579545 / reload_value, rdx = remainder
    cmp rdx,3579545 / 2               ;Is the remainder more than half?
    jb .l3                            ; no, round down
    inc rax                           ; yes, round up
 .l3:
    mov rdi,3
    mov rdx,0                         ;rdx:rax = 3579545 / reload_value
    div rdi                           ;rax = (3579545 / 3) / frequency
    cmp rdx,3 / 2                     ;Is the remainder more than half?
    jb .l4                            ; no, round down
    inc rax                           ; yes, round up
 .l4:
    mov [IRQ0_frequency],rax          ;Store the actual frequency for displaying later
 
 
 ; Calculate the amount of time between IRQs in 32.32 fixed point
 ;
 ; Note: The basic formula is:
 ;           time in ms = reload_value / (3579545 / 3) * 1000
 ;       This can be rearranged in the following way:
 ;           time in ms = reload_value * 3000 / 3579545
 ;           time in ms = reload_value * 3000 / 3579545 * (2^42)/(2^42)
 ;           time in ms = reload_value * 3000 * (2^42) / 3579545 / (2^42)
 ;           time in ms * 2^32 = reload_value * 3000 * (2^42) / 3579545 / (2^42) * (2^32)
 ;           time in ms * 2^32 = reload_value * 3000 * (2^42) / 3579545 / (2^10)
 
    pop rdi                           ;rdi = reload_value
    mov rax,0xDBB3A062                ;rax = 3000 * (2^42) / 3579545
    mul rdi                           ;rdx:rax = reload_value * 3000 * (2^42) / 3579545
    shrd rax,rdx,10
    shr rdx,10                        ;rdx:rax = reload_value * 3000 * (2^42) / 3579545 / (2^10)
 
    mov [IRQ0_ms],rdx                 ;Set whole ms between IRQs
    mov [IRQ0_fractions],rax          ;Set fractions of 1 ms between IRQs
 
 
 ; Program the PIT channel
 
    pushfq
    cli                               ;Disabled interrupts (just in case)
 
    mov al,00110100b                  ;channel 0, lobyte/hibyte, rate generator
    out 0x43, al
 
    mov ax,[PIT_reload_value]         ;ax = 16 bit reload value
    out 0x40,al                       ;Set low byte of PIT reload value
    mov al,ah                         ;ax = high 8 bits of reload value
    out 0x40,al                       ;Set high byte of PIT reload value
 
    popfq
 
    popagrd
    sti
    ret