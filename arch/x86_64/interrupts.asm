section .note.GNU-stack noalloc noexec nowrite progbits

bits 64

section .text

extern scheduler_tick
extern scheduler_switch
extern task_exit
extern syscall_entry
extern gpf_handler
extern double_fault_handler
extern page_fault_handler
extern g_kernel_resume_sp
extern irq_dispatch

global isr_timer
global isr_yield
global isr_ignore
global isr_syscall
global isr_gpf
global isr_double_fault
global isr_page_fault
global task_trampoline

%macro push_regs 0
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
%endmacro

%macro pop_regs 0
    pop r15
    pop r14
    pop r13
    pop r12
    pop r11
    pop r10
    pop r9
    pop r8
    pop rdi
    pop rsi
    pop rbp
    pop rbx
    pop rdx
    pop rcx
    pop rax
%endmacro

isr_timer:
    push_regs

    cld
    mov rdi, rsp
    call scheduler_tick

    mov rsp, rax

    mov al, 0x20
    out 0x20, al

    pop_regs
    iretq

isr_yield:
    push_regs

    cld
    mov rdi, rsp
    call scheduler_switch

    mov rsp, rax

    pop_regs
    iretq

isr_ignore:
    iretq

%macro irq_entry 1
global isr_irq%1
isr_irq%1:
    push_regs
    cld
    mov edi, %1
    call irq_dispatch
    pop_regs
    iretq
%endmacro

%assign irq_number 1
%rep 15
    irq_entry irq_number
%assign irq_number irq_number + 1
%endrep

isr_syscall:
    cmp rax, 2
    je .do_exit

    push_regs
    cld
    mov rdi, rsp
    call syscall_entry
    pop_regs
    iretq

.do_exit:
    mov ax, 0x10
    mov ss, ax
    mov rsp, [g_kernel_resume_sp]
    pop_regs
    pop rbp
    ret

isr_gpf:
    add rsp, 8
    and rsp, -16
    cld
    call gpf_handler
    iretq

isr_double_fault:
    add rsp, 8
    and rsp, -16
    cld
    call double_fault_handler
    iretq

isr_page_fault:
    add rsp, 8
    and rsp, -16
    cld
    call page_fault_handler
    iretq

task_trampoline:
    call rdi
    call task_exit

.hang:
    hlt
    jmp .hang
