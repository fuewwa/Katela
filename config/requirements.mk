MIN_MAKE_VERSION := 3.81
GOALS := $(if $(MAKECMDGOALS),$(MAKECMDGOALS),all)

find_missing = $(strip $(foreach t,$(1),$(if $(shell command -v $(t) 2>/dev/null),,$(t))))
find_first = $(firstword $(foreach t,$(1),$(if $(shell command -v $(t) 2>/dev/null),$(t))))

GRUB_MKRESCUE := $(call find_first,grub-mkrescue grub2-mkrescue)
