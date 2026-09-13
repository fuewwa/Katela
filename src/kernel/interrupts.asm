section .note.GNU-stack noalloc noexec nowrite progbits

section .text

extern scheduler_tick
extern task_exit

global isr_timer
global isr_ignore
global task_trampoline

isr_timer:
    pushad

    push esp
    call scheduler_tick
    add esp, 4

    mov esp, eax

    mov al, 0x20
    out 0x20, al

    popad
    iretd

isr_ignore:
    iretd

task_trampoline:
    call edi
    call task_exit

.hang:
    hlt
    jmp .hang
