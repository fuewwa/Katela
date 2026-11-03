MIN_MAKE_VERSION := 3.81
GOALS := $(if $(MAKECMDGOALS),$(MAKECMDGOALS),all)

find_missing = $(strip $(foreach t,$(1),$(if $(shell command -v $(t) 2>/dev/null),,$(t))))
find_first = $(firstword $(foreach t,$(1),$(if $(shell command -v $(t) 2>/dev/null),$(t))))

GRUB_MKRESCUE := $(call find_first,grub-mkrescue grub2-mkrescue)
GRUB_MKIMAGE := $(call find_first,grub-mkimage grub2-mkimage)
GRUB_BOOT_IMG := $(firstword $(wildcard /usr/lib/grub/i386-pc/boot.img /usr/lib64/grub/i386-pc/boot.img /usr/share/grub/i386-pc/boot.img))

ifneq ($(firstword $(sort $(MAKE_VERSION) $(MIN_MAKE_VERSION))),$(MIN_MAKE_VERSION))
$(error GNU Make $(MIN_MAKE_VERSION) or newer is required, found $(MAKE_VERSION))
endif

ifneq ($(filter-out clean,$(GOALS)),)
MISSING_TOOLS := $(call find_missing,nasm gcc g++ ld)
endif

ifneq ($(filter all iso run run-nvme,$(GOALS)),)
MISSING_TOOLS += $(call find_missing,xorriso)
ifeq ($(GRUB_MKRESCUE),)
MISSING_TOOLS += grub-mkrescue
endif
ifeq ($(GRUB_MKIMAGE),)
MISSING_TOOLS += grub-mkimage
endif
ifeq ($(GRUB_BOOT_IMG),)
MISSING_TOOLS += grub-pc-bin
endif
endif

ifneq ($(filter run run-nvme disk.img,$(GOALS)),)
MISSING_TOOLS += $(call find_missing,qemu-img)
endif

ifneq ($(filter run run-nvme,$(GOALS)),)
MISSING_TOOLS += $(call find_missing,qemu-system-i386)
endif

ifneq ($(strip $(MISSING_TOOLS)),)
$(error Missing required tools: $(strip $(MISSING_TOOLS)). See the Requirements section in README.md)
endif
