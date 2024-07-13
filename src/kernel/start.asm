[bits 32]
; https://www.gnu.org/software/grub/manual/multiboot2/multiboot.pdf
magic equ 0xe85250d6
i386 equ 0
header_len equ (header_end - header_start)
section .multiboot2
header_start:
    dd magic
    dd i386
    dd header_len
    dd -(magic+i386+header_len)

    ; tag 
    dw 0 ; type 
    dw 0 ; flags
    dd 8 ; size
header_end:
section .text
extern console_init
extern gdt_init
extern mem_init
extern kernel_init
global _start
_start:
    mov byte [0xb8000], 'K'
    ;xchg bx, bx
    push ebx ; p ards_counts
    push eax ; magic
    call console_init ; hope stack won't overflow
    call gdt_init
    call mem_init
    call kernel_init
    ;xchg bx, bx
    jmp $
    