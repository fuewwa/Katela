section .note.GNU-stack noalloc noexec nowrite progbits

section .text

extern scheduler_tick
extern task_exit
extern syscall_dispatch
extern gpf_handler
extern g_kernel_resume_esp

global isr_timer
global isr_ignore
global isr_syscall
global isr_gpf
global task_trampoline
global gdt_flush
global tss_flush

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

isr_syscall:
    cmp eax, 2
    je .do_exit

    pushad
    push esp
    call syscall_dispatch
    add esp, 4
    popad
    iretd

.do_exit:
    mov esp, [g_kernel_resume_esp]
    popad
    pop ebp
    ret

isr_gpf:
    add esp, 4
    call gpf_handler
    iretd

gdt_flush:
    mov eax, [esp+4]
    lgdt [eax]

    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax

    jmp 0x08:.flush
.flush:
    ret

tss_flush:
    mov ax, 0x28
    ltr ax
    ret

task_trampoline:
    call edi
    call task_exit

.hang:
    hlt
    jmp .hang
