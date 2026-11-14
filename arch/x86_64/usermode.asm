section .note.GNU-stack noalloc noexec nowrite progbits

bits 64

section .text

extern set_kernel_stack

global enter_usermode

enter_usermode:
    push rbp
    mov rbp, rsp

    push rax
    push rcx
    push rdx
    push rbx
    push rbp
    push rsi
    push rdi
    push r8
    push r9
    push r10
    push r11
    push r12
    push r13
    push r14
    push r15

    mov r12, rdi
    mov r13, rsi
    and r13, -16
    sub r13, 8

    mov rdi, rsp
    sub rsp, 8
    call set_kernel_stack
    add rsp, 8

    push 0x23
    push r13
    push 0x2
    push 0x1B
    push r12

    iretq
