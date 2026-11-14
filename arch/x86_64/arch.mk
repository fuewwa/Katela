CC=gcc
LD=ld -m elf_x86_64 -z max-page-size=0x1000
ARCH_FLAGS=-m64 -mno-sse -mno-mmx -mno-red-zone -fno-pic -fno-pie -fno-asynchronous-unwind-tables
ARCH_INCLUDES=-Iarch/x86_64 -Iarch/x86
ARCH_POST_LINK=objcopy -O elf32-i386 $(KERNEL)
BUNDLE_PTR=.quad
QEMU=qemu-system-x86_64
ISO_IMAGE=katela-x86_64.iso

ARCH_OBJECTS=$(BUILD)/boot.o $(BUILD)/interrupts.o $(BUILD)/gdt_asm.o $(BUILD)/usermode.o \
$(BUILD)/arch.o $(BUILD)/gdt.o $(BUILD)/idt.o $(BUILD)/pic.o $(BUILD)/pit.o \
$(BUILD)/vga.o $(BUILD)/keyboard.o $(BUILD)/speaker.o $(BUILD)/ata.o \
$(BUILD)/recovery.o $(BUILD)/power.o $(BUILD)/installer.o

define ARCH_BUILD
nasm -f elf64 arch/x86_64/boot.asm -o $(BUILD)/boot.o
nasm -f elf64 arch/x86_64/interrupts.asm -o $(BUILD)/interrupts.o
nasm -f elf64 arch/x86_64/gdt.asm -o $(BUILD)/gdt_asm.o
nasm -f elf64 arch/x86_64/usermode.asm -o $(BUILD)/usermode.o
$(CC) $(CFLAGS) -c arch/x86_64/arch.c -o $(BUILD)/arch.o
$(CC) $(CFLAGS) -c arch/x86_64/gdt.c -o $(BUILD)/gdt.o
$(CC) $(CFLAGS) -c arch/x86_64/idt.c -o $(BUILD)/idt.o
$(CC) $(CFLAGS) -c arch/x86/pic.c -o $(BUILD)/pic.o
$(CC) $(CFLAGS) -c arch/x86/pit.c -o $(BUILD)/pit.o
$(CC) $(CFLAGS) -c arch/x86/vga.c -o $(BUILD)/vga.o
$(CC) $(CFLAGS) -c arch/x86/keyboard.c -o $(BUILD)/keyboard.o
$(CC) $(CFLAGS) -c arch/x86/speaker.c -o $(BUILD)/speaker.o
$(CC) $(CFLAGS) -c arch/x86/ata.c -o $(BUILD)/ata.o
$(CC) $(CFLAGS) -c arch/x86/recovery.c -o $(BUILD)/recovery.o
$(CC) $(CFLAGS) -c arch/x86/power.c -o $(BUILD)/power.o
$(CC) $(CFLAGS) -c arch/x86/installer.c -o $(BUILD)/installer.o
endef

include config/pc.mk
