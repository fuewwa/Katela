BUILD=build
ISO=iso
CXX=g++
CXXFLAGS=-m32 -std=c++17 -Wall -Wextra -ffreestanding -mno-sse -mno-mmx -fno-pie -fno-pic -fno-exceptions -fno-rtti -fno-threadsafe-statics -fno-use-cxa-atexit -fno-stack-protector -nostdlib -Iinclude
HOST_CXXFLAGS=-std=c++17 -Wall -Wextra -Iinclude
GRUB_MODULES=biosdisk multiboot boot normal configfile
FACTORY=$(BUILD)/factory.img
