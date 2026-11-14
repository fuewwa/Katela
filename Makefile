ARCH ?= x86_64

include config/requirements.mk
# If you are adding a new dependency
# Make sure to add its existence check to the MISSING_TOOLS block in Makefile
# And add a test that checks the build fails when this tool is missing

include config/config.mk
# Whole variables (if not requirements)
# Paste and create into config.mk

include arch/$(ARCH)/arch.mk

.DEFAULT_GOAL := all

all: $(ALL)

$(KERNEL):
	mkdir -p $(BUILD)
	$(ARCH_BUILD)
	$(CC) $(CFLAGS) -c src/kernel/kernel.c -o $(BUILD)/kernel.o
	$(CC) $(CFLAGS) -fno-strict-aliasing -c src/kernel/fs.c -o $(BUILD)/fs.o
	$(CC) $(CFLAGS) -c src/kernel/mem.c -o $(BUILD)/mem.o
	$(CC) $(CFLAGS) -c src/kernel/mm.c -o $(BUILD)/mm.o
	$(CC) $(CFLAGS) -c src/kernel/panic.c -o $(BUILD)/panic.o
	$(CC) $(CFLAGS) -c src/drivers/framebuffer.c -o $(BUILD)/framebuffer.o
	$(CC) $(CFLAGS) -c src/drivers/font.c -o $(BUILD)/font.o
	$(CC) $(CFLAGS) -c src/drivers/console.c -o $(BUILD)/console.o
	$(CC) $(CFLAGS) -c src/drivers/pci.c -o $(BUILD)/pci.o
	$(CC) $(CFLAGS) -c src/drivers/nvme.c -o $(BUILD)/nvme.o
	$(CC) $(CFLAGS) -c src/drivers/disk.c -o $(BUILD)/disk.o
	$(CC) $(CFLAGS) -c src/drivers/rtl8139.c -o $(BUILD)/rtl8139.o
	$(CC) $(CFLAGS) -c src/kernel/net.c -o $(BUILD)/net.o
	$(CC) $(CFLAGS) -c src/kernel/standart.c -o $(BUILD)/standart.o
	$(CC) $(CFLAGS) -c src/kernel/scheduler.c -o $(BUILD)/scheduler.o
	$(CC) $(CFLAGS) -c src/kernel/irq.c -o $(BUILD)/irq.o
	$(CC) $(CFLAGS) -c src/kernel/syscall.c -o $(BUILD)/syscall.o
	$(CC) $(CFLAGS) -c crypto/lz4.c -o $(BUILD)/lz4.o
	$(CC) $(CFLAGS) -c crypto/lz4hc.c -o $(BUILD)/lz4hc.o
	$(CC) $(CFLAGS) -c crypto/sha256.c -o $(BUILD)/sha256.o
	$(CC) $(CFLAGS) -c crypto/sha1.c -o $(BUILD)/sha1.o
	$(CC) $(CFLAGS) -c crypto/base64.c -o $(BUILD)/base64.o
	$(CC) $(CFLAGS) -c crypto/crc32.c -o $(BUILD)/crc32.o
	$(CC) $(CFLAGS) -c net/addr.c -o $(BUILD)/addr.o
	$(CC) $(CFLAGS) -c net/checksum.c -o $(BUILD)/checksum.o
	$(CC) $(CFLAGS) -c net/ethernet.c -o $(BUILD)/ethernet.o
	$(CC) $(CFLAGS) -c net/arp.c -o $(BUILD)/arp.o
	$(CC) $(CFLAGS) -c net/ipv4.c -o $(BUILD)/ipv4.o
	$(CC) $(CFLAGS) -c net/icmp.c -o $(BUILD)/icmp.o
	$(CC) $(CFLAGS) -c net/udp.c -o $(BUILD)/udp.o
	$(CC) $(CFLAGS) -c net/dns.c -o $(BUILD)/dns.o
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
	echo '.align 8'; \
	echo '.global bundle_table'; \
	echo 'bundle_table:'; \
	i=0; \
	while [ $$i -lt $$n ]; do \
		echo "$(BUNDLE_PTR) .Lname$$i, .Lstart$$i, .Lend$$i"; \
		i=$$((i + 1)); \
	done; \
	echo '$(BUNDLE_PTR) 0, 0, 0'; \
	echo '.section .note.GNU-stack,"",@progbits'; \
	} > $(BUILD)/bundle.S
	$(CC) $(ARCH_FLAGS) -c $(BUILD)/bundle.S -o $(BUILD)/bundle_data.o
	$(CC) $(CFLAGS) -c src/kernel/bundle.c -o $(BUILD)/bundle.o
	$(LD) -T arch/$(ARCH)/linker.ld -o $(KERNEL) -nostdlib \
	$(BUILD)/bundle.o $(BUILD)/bundle_data.o \
	$(BUILD)/kernel.o $(BUILD)/fs.o $(BUILD)/mm.o $(BUILD)/panic.o $(BUILD)/standart.o \
	$(BUILD)/scheduler.o $(BUILD)/syscall.o $(BUILD)/mem.o $(BUILD)/irq.o \
	$(BUILD)/framebuffer.o $(BUILD)/font.o $(BUILD)/console.o \
	$(BUILD)/pci.o $(BUILD)/nvme.o $(BUILD)/disk.o $(BUILD)/rtl8139.o $(BUILD)/net.o \
	$(BUILD)/lz4.o $(BUILD)/lz4hc.o $(BUILD)/sha256.o $(BUILD)/sha1.o $(BUILD)/base64.o $(BUILD)/crc32.o \
	$(BUILD)/addr.o $(BUILD)/checksum.o $(BUILD)/ethernet.o \
	$(BUILD)/arp.o $(BUILD)/ipv4.o $(BUILD)/icmp.o $(BUILD)/udp.o $(BUILD)/dns.o \
	$(ARCH_OBJECTS)
	$(ARCH_POST_LINK)

disk.img:
	qemu-img create -f raw disk.img 16M

clean:
	rm -rf build katela.iso katela-x86_64.iso disk.img $(ISO)/boot/factory.img

.PHONY: all iso run run-nvme clean
# Add Makefile's command names here

