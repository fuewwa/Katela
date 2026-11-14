# Programming Language

Katela is written in the C programming language [1] and assembly for x86 and x86_64. More precisely, the C code is compiled with `gcc` [2] in freestanding mode, for 32-bit x86:

```
gcc -m32 -ffreestanding -nostdlib -fno-stack-protector -Iinclude -Iarch/x86 -c file.c
```

or for x86_64:

```
gcc -m64 -mno-red-zone -fno-pic -fno-pie -ffreestanding -nostdlib -fno-stack-protector -Iinclude -Iarch/x86_64 -Iarch/x86 -c file.c
```

There is no hosted environment. Katela does not link against any standard library, so the kernel provides everything it needs itself, and the compiler is told not to assume otherwise.

## C dialect

The build does not pass a `-std=` option, so the compiler's default dialect is used. On GCC 8 and newer that is `gnu17`, the GNU dialect of ISO C17 [3]. GCC 5 to 7 default to `gnu11`, and anything older defaults to `gnu89`, so a very old compiler will not build Katela.

This dialect contains many extensions to the language [4], and Katela uses some of them as a matter of course, such as attributes and extended inline assembly.

Code should not depend on features newer than C11. If you need a different dialect, discuss it in an issue first, because the choice affects every contributor's toolchain.

Only `gcc` is used to build Katela at the moment. `clang` is not tested and not supported.

## Attributes

Attributes attach implementation-defined meaning to variables, functions, and types without adding new keywords to the language [5].

Katela uses the GNU syntax directly. There are no wrapper macros yet. Two attributes are in common use.

`packed` removes padding from a structure. Use it for any structure whose layout is fixed by hardware or by an on-disk format, such as the GDT and IDT entries:

```c
struct idt_entry {
	unsigned short base_low;
	unsigned short selector;
	unsigned char zero;
	unsigned char flags;
	unsigned short base_high;
} __attribute__((packed));
```

`aligned` forces the alignment of a variable. Katela uses it in two ways: page alignment (`aligned(4096)`) for the NVMe queues and buffers that the controller reads and writes directly, and 4-byte alignment for the filesystem sector buffers:

```c
static unsigned char admin_sq[PAGE_SIZE] __attribute__((aligned(4096)));
```

Do not add an attribute unless there is a reason you can state. Some attributes only affect speed, but `packed` on a hardware structure and `aligned` on a DMA buffer are required for correctness: without them the code can misbehave even though it compiles.

Write the full `__attribute__((...))` form.

## Inline assembly

Katela uses GCC extended inline assembly in AT&T syntax. Every statement in the tree is written as `asm volatile`, and the statements that depend on the processor mode live in `arch/`. The main uses are:

- port I/O: `inb`, `inw`, `inl`, `outb`, `outw`, `outl`
- interrupt control and halting: `cli`, `sti`, `hlt`
- saving and restoring the flags register around critical sections (`pushfl` and `popfl` on x86, `pushfq` and `popfq` on x86_64)
- loading CPU tables: `lidt`
- software interrupts: `int $0x80` for syscalls and `int $0x81`, which the scheduler uses

```c
asm volatile("cli");
asm volatile("lidt %0" : : "m"(pointer));
```

`volatile` tells the compiler not to delete the statement or to treat it as having no side effects. It does not stop the compiler from moving ordinary memory accesses across it. When the statement must act as a barrier, add a `"memory"` clobber, as the scheduler does when it saves and restores the flags:

```c
asm volatile("pushfl; popl %0; cli" : "=r"(flags) : : "memory");
```

Use inline assembly for short sequences that C cannot express. Katela's inline assembly is a handful of instructions at most, and longer sequences belong in an assembly file.

## Assembly files

Assembly source is written for NASM [6]. The x86 files are assembled as 32-bit ELF:

```
nasm -f elf32 file.asm -o file.o
```

and the x86_64 files as 64-bit ELF:

```
nasm -f elf64 file.asm -o file.o
```

The x86_64 boot code is the exception inside the 64-bit files: GRUB starts the kernel in 32-bit protected mode, so the first part of `arch/x86_64/boot.asm` is written with `bits 32`. It builds the page tables, enters long mode, and continues with `bits 64`. The x86_64 kernel is linked as a 64-bit ELF file and then converted with `objcopy -O elf32-i386`, because the Multiboot loader of GRUB only reads 32-bit ELF files.

NASM uses Intel syntax. Inline assembly in C files uses AT&T syntax, because that is what GCC expects. Keep the two apart. Do not mix syntaxes inside one file.

Every `.asm` file contains a `.note.GNU-stack` section so the linker does not mark the stack executable:

```
section .note.GNU-stack noalloc noexec nowrite progbits
```

Code that needs the exact layout of the machine, such as the boot entry point, the Multiboot header, interrupt stubs, and the switch to usermode, belongs in assembly. Everything else belongs in C.

## Other languages

Katela does not currently use C++ or Rust. Build scripts and helper tools may use shell, Make, or Python, but nothing in the kernel image itself is written in them.

## References

1. [C language standards (ISO WG14)](https://www.open-std.org/jtc1/sc22/wg14/www/standards)
2. [GCC](https://gcc.gnu.org)
3. [GCC C dialect options](https://gcc.gnu.org/onlinedocs/gcc/C-Dialect-Options.html)
4. [GCC C extensions](https://gcc.gnu.org/onlinedocs/gcc/C-Extensions.html)
5. [GCC attribute syntax](https://gcc.gnu.org/onlinedocs/gcc/Attribute-Syntax.html)
6. [NASM](https://www.nasm.us)
