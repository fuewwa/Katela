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
	gcc -m32 -ffreestanding -mno-sse -mno-mmx -Iinclude -nostdlib -fno-stack-protector -c src/kernel/kernel.c -o $(BUILD)/kernel.o
	gcc -m32 -ffreestanding -mno-sse -mno-mmx -fno-strict-aliasing -Iinclude -nostdlib -fno-stack-protector -c src/kernel/fs.c -o $(BUILD)/fs.o
	gcc -m32 -ffreestanding -mno-sse -mno-mmx -Iinclude -nostdlib -fno-stack-protector -c src/kernel/mem.c -o $(BUILD)/mem.o
	gcc -m32 -ffreestanding -mno-sse -mno-mmx -Iinclude -nostdlib -fno-stack-protector -c src/kernel/mm.c -o $(BUILD)/mm.o
	gcc -m32 -ffreestanding -mno-sse -mno-mmx -Iinclude -nostdlib -fno-stack-protector -c src/kernel/panic.c -o $(BUILD)/panic.o
	gcc -m32 -ffreestanding -mno-sse -mno-mmx -Iinclude -nostdlib -fno-stack-protector -c src/drivers/vga.c -o $(BUILD)/vga.o
	gcc -m32 -ffreestanding -mno-sse -mno-mmx -Iinclude -nostdlib -fno-stack-protector -c src/drivers/keyboard.c -o $(BUILD)/keyboard.o
	gcc -m32 -ffreestanding -mno-sse -mno-mmx -Iinclude -nostdlib -fno-stack-protector -c src/drivers/speaker.c -o $(BUILD)/speaker.o
	gcc -m32 -ffreestanding -mno-sse -mno-mmx -Iinclude -nostdlib -fno-stack-protector -c src/drivers/ata.c -o $(BUILD)/ata.o
	gcc -m32 -ffreestanding -mno-sse -mno-mmx -Iinclude -nostdlib -fno-stack-protector -c src/drivers/pci.c -o $(BUILD)/pci.o
	gcc -m32 -ffreestanding -mno-sse -mno-mmx -Iinclude -nostdlib -fno-stack-protector -c src/drivers/nvme.c -o $(BUILD)/nvme.o
	gcc -m32 -ffreestanding -mno-sse -mno-mmx -Iinclude -nostdlib -fno-stack-protector -c src/drivers/disk.c -o $(BUILD)/disk.o
	gcc -m32 -ffreestanding -mno-sse -mno-mmx -Iinclude -nostdlib -fno-stack-protector -c src/drivers/rtl8139.c -o $(BUILD)/rtl8139.o
	gcc -m32 -ffreestanding -mno-sse -mno-mmx -Iinclude -nostdlib -fno-stack-protector -c src/kernel/net.c -o $(BUILD)/net.o
	gcc -m32 -ffreestanding -mno-sse -mno-mmx -Iinclude -nostdlib -fno-stack-protector -c src/kernel/standart.c -o $(BUILD)/standart.o
	gcc -m32 -ffreestanding -mno-sse -mno-mmx -Iinclude -nostdlib -fno-stack-protector -c src/boot/idt.c -o $(BUILD)/idt.o
	gcc -m32 -ffreestanding -mno-sse -mno-mmx -Iinclude -nostdlib -fno-stack-protector -c src/boot/pic.c -o $(BUILD)/pic.o
	gcc -m32 -ffreestanding -mno-sse -mno-mmx -Iinclude -nostdlib -fno-stack-protector -c src/boot/pit.c -o $(BUILD)/pit.o
	gcc -m32 -ffreestanding -mno-sse -mno-mmx -Iinclude -nostdlib -fno-stack-protector -c src/boot/irq.c -o $(BUILD)/irq.o
	gcc -m32 -ffreestanding -mno-sse -mno-mmx -Iinclude -nostdlib -fno-stack-protector -c src/kernel/scheduler.c -o $(BUILD)/scheduler.o
	gcc -m32 -ffreestanding -mno-sse -mno-mmx -Iinclude -nostdlib -fno-stack-protector -c src/boot/gdt.c -o $(BUILD)/gdt.o
	gcc -m32 -ffreestanding -mno-sse -mno-mmx -Iinclude -nostdlib -fno-stack-protector -c src/kernel/syscall.c -o $(BUILD)/syscall.o
	gcc -m32 -ffreestanding -mno-sse -mno-mmx -Iinclude -nostdlib -fno-stack-protector -c crypto/lz4.c -o $(BUILD)/lz4.o
	gcc -m32 -ffreestanding -mno-sse -mno-mmx -Iinclude -nostdlib -fno-stack-protector -c crypto/lz4hc.c -o $(BUILD)/lz4hc.o
	gcc -m32 -ffreestanding -mno-sse -mno-mmx -Iinclude -nostdlib -fno-stack-protector -c crypto/sha256.c -o $(BUILD)/sha256.o
	gcc -m32 -ffreestanding -mno-sse -mno-mmx -Iinclude -nostdlib -fno-stack-protector -c crypto/sha1.c -o $(BUILD)/sha1.o
	gcc -m32 -ffreestanding -mno-sse -mno-mmx -Iinclude -nostdlib -fno-stack-protector -c crypto/base64.c -o $(BUILD)/base64.o
	gcc -m32 -ffreestanding -mno-sse -mno-mmx -Iinclude -nostdlib -fno-stack-protector -c crypto/crc32.c -o $(BUILD)/crc32.o
	gcc -m32 -ffreestanding -mno-sse -mno-mmx -Iinclude -nostdlib -fno-stack-protector -c net/addr.c -o $(BUILD)/addr.o
	gcc -m32 -ffreestanding -mno-sse -mno-mmx -Iinclude -nostdlib -fno-stack-protector -c net/checksum.c -o $(BUILD)/checksum.o
	gcc -m32 -ffreestanding -mno-sse -mno-mmx -Iinclude -nostdlib -fno-stack-protector -c net/ethernet.c -o $(BUILD)/ethernet.o
	gcc -m32 -ffreestanding -mno-sse -mno-mmx -Iinclude -nostdlib -fno-stack-protector -c net/arp.c -o $(BUILD)/arp.o
	gcc -m32 -ffreestanding -mno-sse -mno-mmx -Iinclude -nostdlib -fno-stack-protector -c net/ipv4.c -o $(BUILD)/ipv4.o
	gcc -m32 -ffreestanding -mno-sse -mno-mmx -Iinclude -nostdlib -fno-stack-protector -c net/icmp.c -o $(BUILD)/icmp.o
	gcc -m32 -ffreestanding -mno-sse -mno-mmx -Iinclude -nostdlib -fno-stack-protector -c net/udp.c -o $(BUILD)/udp.o
	gcc -m32 -ffreestanding -mno-sse -mno-mmx -Iinclude -nostdlib -fno-stack-protector -c net/dns.c -o $(BUILD)/dns.o
	{ \
	echo '.section .rodata'; \
	n=0; \
	for f in $(ISO)/bin/*; do \
		[ -f "$$f" ] || continue; \
		echo ".Lname$$n: .asciz \"$$(basename "$$f")\""; \
		echo ".Lstart$$n: .incbin \"$$f\""; \
		echo ".Lend$$n:"; \
		n=$$((n + 1)); \
	done; \
	echo '.align 4'; \
	echo '.global bundle_table'; \
	echo 'bundle_table:'; \
	i=0; \
	while [ $$i -lt $$n ]; do \
		echo ".long .Lname$$i, .Lstart$$i, .Lend$$i"; \
		i=$$((i + 1)); \
	done; \
	echo '.long 0, 0, 0'; \
	echo '.section .note.GNU-stack,"",@progbits'; \
	} > $(BUILD)/bundle.S
	gcc -m32 -c $(BUILD)/bundle.S -o $(BUILD)/bundle_data.o
	gcc -m32 -ffreestanding -mno-sse -mno-mmx -Iinclude -nostdlib -fno-stack-protector -c src/kernel/bundle.c -o $(BUILD)/bundle.o
	ld -m elf_i386 -T linker.ld -o $(BUILD)/kernel.bin -nostdlib \
	$(BUILD)/bundle.o $(BUILD)/bundle_data.o \
	$(BUILD)/boot.o $(BUILD)/kernel.o $(BUILD)/fs.o $(BUILD)/mm.o \
	$(BUILD)/panic.o $(BUILD)/vga.o $(BUILD)/speaker.o $(BUILD)/keyboard.o $(BUILD)/standart.o \
	$(BUILD)/idt.o $(BUILD)/scheduler.o $(BUILD)/interrupts.o $(BUILD)/ata.o \
	$(BUILD)/gdt.o $(BUILD)/syscall.o $(BUILD)/usermode.o $(BUILD)/mem.o $(BUILD)/irq.o \
	$(BUILD)/pci.o $(BUILD)/nvme.o $(BUILD)/disk.o $(BUILD)/rtl8139.o $(BUILD)/net.o \
	$(BUILD)/gdt_asm.o $(BUILD)/pic.o $(BUILD)/pit.o \
	$(BUILD)/lz4.o $(BUILD)/lz4hc.o $(BUILD)/sha256.o $(BUILD)/sha1.o $(BUILD)/base64.o $(BUILD)/crc32.o \
	$(BUILD)/addr.o $(BUILD)/checksum.o $(BUILD)/ethernet.o \
	$(BUILD)/arp.o $(BUILD)/ipv4.o $(BUILD)/icmp.o $(BUILD)/udp.o $(BUILD)/dns.o

iso: $(BUILD)/kernel.bin
	mkdir -p $(ISO)/boot
	cp $(BUILD)/kernel.bin $(ISO)/boot/
	$(GRUB_MKRESCUE) -o katela.iso $(ISO)

disk.img:
	qemu-img create -f raw disk.img 16M

run: iso disk.img
	qemu-system-i386 -cdrom katela.iso -hda disk.img -netdev user,id=net0 -device rtl8139,netdev=net0 -machine pcspk-audiodev=snd -audiodev alsa,id=snd

run-nvme: iso disk.img
	qemu-system-i386 -cdrom katela.iso -drive file=disk.img,if=none,id=nvm,format=raw -device nvme,serial=katela,drive=nvm -netdev user,id=net0 -device rtl8139,netdev=net0 -machine pcspk-audiodev=snd -audiodev alsa,id=snd

clean:
	rm -rf build katela.iso disk.img

.PHONY: all iso run run-nvme clean
# Add Makefile's command names here

