$(BUILD)/kernel.iso: $(BUILD)/kernel.bin $(SRC)/utils/grub.cfg
# kernel.bin 需要合法
	grub-file --is-x86-multiboot2 $<
# 创建iso目录
	mkdir -p $(BUILD)/iso/boot/grub
# 拷贝内核文件
	cp $< $(BUILD)/iso/boot
# 拷贝grub配置文件
	cp $(SRC)/utils/grub.cfg $(BUILD)/iso/boot/grub
# 生成iso文件
	grub-mkrescue -o $@ $(BUILD)/iso

.PHONY: bochsb
bochsb:$(BUILD)/kernel.iso
	bochs -q -f ../bochs/bochsrc.grub -unlock

QEMU+= -drive file=$(BUILD)/kernel.iso,media=cdrom

QEMU_CDROM:=-boot d

.PHONY: qemub
qemub:$(BUILD)/kernel.iso
	$(QEMU) $(QEMU_CDROM) \
	$(QEMU_DEBUG)

.PHONY: cdrom
cdrom: $(BUILD)/kernel.iso $(IMAGES)