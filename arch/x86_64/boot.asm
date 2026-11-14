section .multiboot
align 4
dd 0x1BADB002
dd 0x4
dd -(0x1BADB002 + 0x4)
dd 0
dd 0
dd 0
dd 0
dd 0
dd 0
dd 1024
dd 768
dd 32

section .note.GNU-stack noalloc noexec nowrite progbits

section .bss
align 4096
pml4:
resb 4096
pdpt:
resb 4096
page_directories:
resb 16384

align 16
stack:
resb 32768
stack_top:

global multiboot_magic
global multiboot_info
multiboot_magic: resd 1
multiboot_info: resd 1

section .rodata
align 16
gdt64:
dq 0
dq 0x00AF9A000000FFFF
dq 0x00CF92000000FFFF
gdt64_end:

gdt64_pointer:
dw gdt64_end - gdt64 - 1
dd gdt64

no_long_mode_text:
db "This CPU does not support 64-bit mode", 0

section .text
global _start
extern kernel_main

bits 32

_start:
    cli

    mov edi, eax
    mov esi, ebx

    mov esp, stack_top

    pushfd
    pop eax
    mov ecx, eax
    xor eax, 1 << 21
    push eax
    popfd
    pushfd
    pop eax
    push ecx
    popfd
    xor eax, ecx
    jz no_long_mode

    mov eax, 0x80000000
    cpuid
    cmp eax, 0x80000001
    jb no_long_mode

    mov eax, 0x80000001
    cpuid
    test edx, 1 << 29
    jz no_long_mode

    mov eax, pdpt
    or eax, 7
    mov [pml4], eax
    mov dword [pml4 + 4], 0

    xor ecx, ecx
.map_pdpt:
    mov eax, ecx
    shl eax, 12
    add eax, page_directories
    or eax, 7
    mov [pdpt + ecx * 8], eax
    mov dword [pdpt + ecx * 8 + 4], 0
    inc ecx
    cmp ecx, 4
    jb .map_pdpt

    xor ecx, ecx
.map_pd:
    mov eax, ecx
    shl eax, 21
    or eax, 0x87
    mov [page_directories + ecx * 8], eax
    mov dword [page_directories + ecx * 8 + 4], 0
    inc ecx
    cmp ecx, 2048
    jb .map_pd

    mov eax, cr4
    or eax, 1 << 5
    mov cr4, eax

    mov eax, pml4
    mov cr3, eax

    mov ecx, 0xC0000080
    rdmsr
    or eax, 1 << 8
    wrmsr

    mov eax, cr0
    or eax, 1 << 31
    mov cr0, eax

    lgdt [gdt64_pointer]

    push 0x08
    push long_mode
    retf

no_long_mode:
    mov edx, 0xB8000
    mov ecx, no_long_mode_text
.print:
    mov al, [ecx]
    test al, al
    jz .halt
    mov [edx], al
    mov byte [edx + 1], 0x4F
    inc ecx
    add edx, 2
    jmp .print
.halt:
    hlt
    jmp .halt

bits 64

long_mode:
    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax

    mov rsp, stack_top

    mov [multiboot_magic], edi
    mov [multiboot_info], esi

    xor ebp, ebp
    call kernel_main

.hang:
    hlt
    jmp .hang
