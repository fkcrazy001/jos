; this file was for interrupt handler
[bits 32]

extern printk

section .text

global interrupt_handler

interrupt_handler:
    ;xchg bx, bx ; bochs break point

    push default_handler_msg
    call printk
    add esp, 4

    iret

section .data
default_handler_msg:
    db "msg from default handler", 10, 0