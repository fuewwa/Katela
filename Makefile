BUILD=build
ISO=iso

all: iso

$(BUILD)/kernel.bin:
	mkdir -p $(BUILD)
	nasm -f elf32 src/boot/boot.asm -o $(BUILD)/boot.o
	nasm -f elf32 src/kernel/interrupts.asm -o $(BUILD)/interrupts.o
	gcc -m32 -ffreestanding -Iinclude -nostdlib -fno-stack-protector -c src/kernel/kernel.c -o $(BUILD)/kernel.o
	gcc -m32 -ffreestanding -Iinclude -nostdlib -fno-stack-protector -c src/kernel/fs.c -o $(BUILD)/fs.o
	gcc -m32 -ffreestanding -Iinclude -nostdlib -fno-stack-protector -c src/kernel/mm.c -o $(BUILD)/mm.o
	gcc -m32 -ffreestanding -Iinclude -nostdlib -fno-stack-protector -c src/kernel/panic.c -o $(BUILD)/panic.o
	gcc -m32 -ffreestanding -Iinclude -nostdlib -fno-stack-protector -c src/drivers/vga.c -o $(BUILD)/vga.o
	gcc -m32 -ffreestanding -Iinclude -nostdlib -fno-stack-protector -c src/drivers/keyboard.c -o $(BUILD)/keyboard.o
	gcc -m32 -ffreestanding -Iinclude -nostdlib -fno-stack-protector -c src/drivers/speaker.c -o $(BUILD)/speaker.o
	gcc -m32 -ffreestanding -Iinclude -nostdlib -fno-stack-protector -c src/drivers/ata.c -o $(BUILD)/ata.o
	gcc -m32 -ffreestanding -Iinclude -nostdlib -fno-stack-protector -c src/kernel/standart.c -o $(BUILD)/standart.o
	gcc -m32 -ffreestanding -Iinclude -nostdlib -fno-stack-protector -c src/kernel/idt.c -o $(BUILD)/idt.o
	gcc -m32 -ffreestanding -Iinclude -nostdlib -fno-stack-protector -c src/kernel/scheduler.c -o $(BUILD)/scheduler.o
	ld -m elf_i386 -T linker.ld -o $(BUILD)/kernel.bin -nostdlib \
	$(BUILD)/boot.o $(BUILD)/kernel.o $(BUILD)/fs.o $(BUILD)/mm.o \
	$(BUILD)/panic.o $(BUILD)/vga.o $(BUILD)/speaker.o $(BUILD)/keyboard.o $(BUILD)/standart.o \
	$(BUILD)/idt.o $(BUILD)/scheduler.o $(BUILD)/interrupts.o $(BUILD)/ata.o

iso: $(BUILD)/kernel.bin
	mkdir -p $(ISO)/boot
	cp $(BUILD)/kernel.bin $(ISO)/boot/
	grub-mkrescue -o katela.iso $(ISO)

disk.img:
	qemu-img create -f raw disk.img 16M

run: iso disk.img
	qemu-system-i386 -cdrom katela.iso -hda disk.img -machine pcspk-audiodev=snd -audiodev alsa,id=snd

clean:
	rm -rf build katela.iso disk.img
