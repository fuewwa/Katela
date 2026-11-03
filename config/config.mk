BUILD=build
ISO=iso
GRUB_MODULES=biosdisk multiboot boot normal configfile
FACTORY=$(BUILD)/factory.img
CFLAGS=-m32 -ffreestanding -mno-sse -mno-mmx -Iinclude -nostdlib -fno-stack-protector
