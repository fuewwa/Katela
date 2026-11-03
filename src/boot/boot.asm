section .multiboot
align 4
dd 0x1BADB002
dd 0x0
dd -(0x1BADB002)

section .note.GNU-stack noalloc noexec nowrite progbits

section .bss
align 16
stack:
resb 16384

global multiboot_magic
global multiboot_info
multiboot_magic: resd 1
multiboot_info: resd 1

section .text
global _start
extern kernel_main

_start:
    cli

    mov [multiboot_magic], eax
    mov [multiboot_info], ebx

    mov esp, stack + 16384

    call kernel_main

.hang:
    hlt
    jmp .hang
