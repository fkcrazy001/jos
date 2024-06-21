[bits 32]
extern console_init
extern mem_init
extern kernel_init
global _start
_start:
    mov byte [0xb8000], 'K'
    ;xchg bx, bx
    push ebx ; p ards_counts
    push eax ; magic
    call console_init ; hope stack won't overflow
    call mem_init
    ; call kernel_init
    ;xchg bx, bx
    jmp $
    