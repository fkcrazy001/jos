[bits 32]


global task_switch

task_switch:
    push ebp
    mov ebp, esp
    ; 通用寄存器保存
    push ebx;
    push esi;
    push edi;

    ; 保存当前程序的栈地址
    ; current
    mov eax, esp
    and eax, 0xfffff000
    ; current-> stk = esp ; stk == current
    mov [eax], esp

    ; 将下一个程序启动起来
    ; next
    mov eax, [ebp + 8]
    ; esp = next->stk; stk == next
    mov esp, [eax]
    ; 恢复通用栈
    pop edi
    pop esi
    pop edx
    pop ebp

    ret