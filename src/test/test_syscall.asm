[bits 32]

section .text

global main
main:
    mov eax, 4 ;write
    mov ebx, 1 ; stdout
    mov ecx, messages
    mov edx, end-messages
    int 0x80

    ; do exit 
    mov eax, 1; exit
    mov ebx, 0
    int 0x80

section .data
messages:
    db "hello world!!!", 10,13,0
end: