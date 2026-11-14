ifeq ($(ARCH),x86)
BUILD=build
else
BUILD=build/$(ARCH)
endif
ISO=iso
KERNEL=$(BUILD)/kernel.bin
CFLAGS=$(ARCH_FLAGS) -ffreestanding -Iinclude $(ARCH_INCLUDES) -nostdlib -fno-stack-protector
