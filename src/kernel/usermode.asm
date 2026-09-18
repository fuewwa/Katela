section .note.GNU-stack noalloc noexec nowrite progbits

section .text

extern set_kernel_stack

global enter_usermode

enter_usermode:
    push ebp
    mov ebp, esp
    pushad

    push esp
    call set_kernel_stack
    add esp, 4

    mov eax, [ebp+8]
    mov ecx, [ebp+12]

    push 0x23
    push ecx
    push 0x2
    push 0x1B
    push eax

    mov ax, 0x23
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax

    iretd
