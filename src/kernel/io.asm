[bits 32]

section .text

global inb

inb:
    push ebp
    mov ebp, esp
    
    xor eax, eax

    mov edx, [ebp + 8] ; port
    in al, dx ; 低8位到ax

    jmp $+2
    jmp $+2
    jmp $+2 ; delay

    leave
    ret

inw:
    push ebp
    mov ebp, esp
    
    xor eax, eax

    mov edx, [ebp + 8] ; port
    in ax, dx ; 低16位到ax

    jmp $+2
    jmp $+2
    jmp $+2 ; delay

    leave
    ret

global outb
outb:
    push ebp
    mov ebp, esp
    
    mov edx, [ebp + 8]; port
    mov eax, [ebp + 12] ; value
    out dx, al ; al->port dx
    
    jmp $+2
    jmp $+2
    jmp $+2 ; delay

    leave
    ret


global outw
outw:
    push ebp
    mov ebp, esp
    
    mov edx, [ebp + 8]; port
    mov eax, [ebp + 12] ; value
    out dx, ax ; al->port dx
    
    jmp $+2
    jmp $+2
    jmp $+2 ; delay

    leave
    ret