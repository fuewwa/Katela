GRUB_MODULES=biosdisk multiboot boot normal configfile all_video
FACTORY=$(BUILD)/factory.img
ALL=iso

$(FACTORY): $(KERNEL) tools/mkfactory.py include/layout.h
	python3 tools/mkfactory.py config include/layout.h $(BUILD)/embedded.cfg $(BUILD)/disk.cfg
	$(GRUB_MKIMAGE) -O i386-pc -p '(hd0)' -c $(BUILD)/embedded.cfg -o $(BUILD)/core.img $(GRUB_MODULES)
	python3 tools/mkfactory.py image include/layout.h $(GRUB_BOOT_IMG) $(BUILD)/core.img $(KERNEL) $(BUILD)/disk.cfg $(FACTORY)

iso: $(KERNEL) $(FACTORY)
	mkdir -p $(ISO)/boot
	cp $(KERNEL) $(ISO)/boot/
	cp $(FACTORY) $(ISO)/boot/factory.img
	$(GRUB_MKRESCUE) -o $(ISO_IMAGE) $(ISO)

run: iso disk.img
	$(QEMU) -cdrom $(ISO_IMAGE) -hda disk.img -netdev user,id=net0 -device rtl8139,netdev=net0 -machine pcspk-audiodev=snd -audiodev alsa,id=snd

run-nvme: iso disk.img
	$(QEMU) -cdrom $(ISO_IMAGE) -drive file=disk.img,if=none,id=nvm,format=raw -device nvme,serial=katela,drive=nvm -netdev user,id=net0 -device rtl8139,netdev=net0 -machine pcspk-audiodev=snd -audiodev alsa,id=snd
