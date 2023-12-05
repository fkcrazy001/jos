[bits 32]
extern exit
global main
main:
    push 5
    ;pusha ; 八个基础寄存器的值
    ;popa ; 七个基础寄存器的值， esp忽略, esp是重新计算出来的
    push 0
    call exit