section .note.GNU-stack noalloc noexec nowrite progbits

bits 64

section .text

global gdt_flush
global tss_flush

gdt_flush:
    lgdt [rdi]

    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax

    pop rax
    push 0x08
    push rax
    retfq

tss_flush:
    mov ax, 0x28
    ltr ax
    ret
