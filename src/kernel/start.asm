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
extern gdt_ptr
; multiboot2 setting s when invoke _start
; eax = 0x36d76289
; ebx = (multiboot2*)addr;
; cs: 32位rp代码段
; DS/ES/FS/GS/SS: 32位rw data
; A20启用
; CR0: PG=0, PE=1
; EFLAGS: VM=0(VIRTUAL 8086 MODE), IF=0
; ESP 内核需要尽早切换
; GDTR 内核需要尽早切换
; IDTR 未定义，if使能之前得定位完成

code_selector equ (1<<3)
data_selector equ (2<<3)
global _start
_start:
    mov byte [0xb8000], 'K'
    xchg bx, bx
    push ebx ; p ards_counts
    push eax ; magic
    call console_init ; hope stack won't overflow
    call gdt_init
    
    lgdt [gdt_ptr]
    jmp dword code_selector:_next
_next 
    mov ax, data_selector
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax

    call mem_init
    mov esp, 0x10000
    call kernel_init
    ;xchg bx, bx
    jmp $
    