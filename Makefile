include config/requirements.mk
# If you are adding a new dependency
# Make sure to add its existence check to the MISSING_TOOLS block in Makefile
# And add a test that checks the build fails when this tool is missing

include config/config.mk
# Whole variables (if not requirements)
# Paste and create into config.mk

all: iso

$(BUILD)/kernel.bin:
	mkdir -p $(BUILD)
	nasm -f elf32 src/boot/boot.asm -o $(BUILD)/boot.o
	nasm -f elf32 src/boot/interrupts.asm -o $(BUILD)/interrupts.o
	nasm -f elf32 src/boot/gdt.asm -o $(BUILD)/gdt_asm.o
	nasm -f elf32 src/kernel/usermode.asm -o $(BUILD)/usermode.o
	gcc -m32 -ffreestanding -Iinclude -nostdlib -fno-stack-protector -c src/kernel/kernel.c -o $(BUILD)/kernel.o
	gcc -m32 -ffreestanding -fno-strict-aliasing -Iinclude -nostdlib -fno-stack-protector -c src/kernel/fs.c -o $(BUILD)/fs.o
	gcc -m32 -ffreestanding -Iinclude -nostdlib -fno-stack-protector -c src/kernel/mem.c -o $(BUILD)/mem.o
	gcc -m32 -ffreestanding -Iinclude -nostdlib -fno-stack-protector -c src/kernel/mm.c -o $(BUILD)/mm.o
	gcc -m32 -ffreestanding -Iinclude -nostdlib -fno-stack-protector -c src/kernel/panic.c -o $(BUILD)/panic.o
	gcc -m32 -ffreestanding -Iinclude -nostdlib -fno-stack-protector -c src/drivers/vga.c -o $(BUILD)/vga.o
	gcc -m32 -ffreestanding -Iinclude -nostdlib -fno-stack-protector -c src/drivers/keyboard.c -o $(BUILD)/keyboard.o
	gcc -m32 -ffreestanding -Iinclude -nostdlib -fno-stack-protector -c src/drivers/speaker.c -o $(BUILD)/speaker.o
	gcc -m32 -ffreestanding -Iinclude -nostdlib -fno-stack-protector -c src/drivers/ata.c -o $(BUILD)/ata.o
	gcc -m32 -ffreestanding -Iinclude -nostdlib -fno-stack-protector -c src/drivers/pci.c -o $(BUILD)/pci.o
	gcc -m32 -ffreestanding -Iinclude -nostdlib -fno-stack-protector -c src/drivers/nvme.c -o $(BUILD)/nvme.o
	gcc -m32 -ffreestanding -Iinclude -nostdlib -fno-stack-protector -c src/drivers/disk.c -o $(BUILD)/disk.o
	gcc -m32 -ffreestanding -Iinclude -nostdlib -fno-stack-protector -c src/kernel/standart.c -o $(BUILD)/standart.o
	gcc -m32 -ffreestanding -Iinclude -nostdlib -fno-stack-protector -c src/boot/idt.c -o $(BUILD)/idt.o
	gcc -m32 -ffreestanding -Iinclude -nostdlib -fno-stack-protector -c src/boot/pic.c -o $(BUILD)/pic.o
	gcc -m32 -ffreestanding -Iinclude -nostdlib -fno-stack-protector -c src/boot/pit.c -o $(BUILD)/pit.o
	gcc -m32 -ffreestanding -Iinclude -nostdlib -fno-stack-protector -c src/kernel/scheduler.c -o $(BUILD)/scheduler.o
	gcc -m32 -ffreestanding -Iinclude -nostdlib -fno-stack-protector -c src/boot/gdt.c -o $(BUILD)/gdt.o
	gcc -m32 -ffreestanding -Iinclude -nostdlib -fno-stack-protector -c src/kernel/syscall.c -o $(BUILD)/syscall.o
	gcc -m32 -ffreestanding -Iinclude -nostdlib -fno-stack-protector -c crypto/lz4.c -o $(BUILD)/lz4.o
	gcc -m32 -ffreestanding -Iinclude -nostdlib -fno-stack-protector -c crypto/lz4hc.c -o $(BUILD)/lz4hc.o
	gcc -m32 -ffreestanding -Iinclude -nostdlib -fno-stack-protector -c crypto/sha256.c -o $(BUILD)/sha256.o
	gcc -m32 -ffreestanding -Iinclude -nostdlib -fno-stack-protector -c crypto/sha1.c -o $(BUILD)/sha1.o
	gcc -m32 -ffreestanding -Iinclude -nostdlib -fno-stack-protector -c net/addr.c -o $(BUILD)/addr.o
	ld -m elf_i386 -T linker.ld -o $(BUILD)/kernel.bin -nostdlib \
	$(BUILD)/boot.o $(BUILD)/kernel.o $(BUILD)/fs.o $(BUILD)/mm.o \
	$(BUILD)/panic.o $(BUILD)/vga.o $(BUILD)/speaker.o $(BUILD)/keyboard.o $(BUILD)/standart.o \
	$(BUILD)/idt.o $(BUILD)/scheduler.o $(BUILD)/interrupts.o $(BUILD)/ata.o \
	$(BUILD)/gdt.o $(BUILD)/syscall.o $(BUILD)/usermode.o $(BUILD)/mem.o \
	$(BUILD)/pci.o $(BUILD)/nvme.o $(BUILD)/disk.o \
	$(BUILD)/gdt_asm.o $(BUILD)/pic.o $(BUILD)/pit.o \
	$(BUILD)/lz4.o $(BUILD)/lz4hc.o $(BUILD)/sha256.o $(BUILD)/sha1.o \
	$(BUILD)/addr.o

iso: $(BUILD)/kernel.bin
	mkdir -p $(ISO)/boot
	cp $(BUILD)/kernel.bin $(ISO)/boot/
	$(GRUB_MKRESCUE) -o katela.iso $(ISO)

disk.img:
	qemu-img create -f raw disk.img 16M

run: iso disk.img
	qemu-system-i386 -cdrom katela.iso -hda disk.img -machine pcspk-audiodev=snd -audiodev alsa,id=snd

run-nvme: iso disk.img
	qemu-system-i386 -cdrom katela.iso -drive file=disk.img,if=none,id=nvm,format=raw -device nvme,serial=katela,drive=nvm -machine pcspk-audiodev=snd -audiodev alsa,id=snd

clean:
	rm -rf build katela.iso disk.img

.PHONY: all iso run run-nvme clean
# Add Makefile's command names here

