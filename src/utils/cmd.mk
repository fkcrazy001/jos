
.PHONY: bochs
bochs:$(IMAGES)
	bochs -q -f ../bochs/bochsrc

.PHONY: bochsg
bochsg:$(IMAGES)
	bochs-gdb -q -f ../bochs/bochsrc.gdb

QEMU:= qemu-system-i386 #i386
QEMU+= -m 32M  # 32M mm
QEMU+= -audiodev pa,id=hda # 音频设备 
QEMU+= -machine pcspk-audiodev=hda  # pc speaker设备
QEMU+= -rtc base=localtime  #设备本地时间
QEMU+= -drive file=$(BUILD)/master.img,if=ide,index=0,media=disk,format=raw #主硬盘
QEMU+= -drive file=$(BUILD)/slave.img,if=ide,index=1,media=disk,format=raw # 从硬盘

QEMU_DISK:=-boot c

QEMU_DEBUG:= -s -S

.PHONY: qemu
qemu:$(IMAGES)
	$(QEMU) $(QEMU_DISK)

.PHONY: qemug
qemug:$(IMAGES)
	$(QEMU) $(QEMU_DISK) $(QEMU_DEBUG)

$(BUILD)/master.vmdk:$(BUILD)/master.img
	qemu-img convert -pO vmdk $< $@
.PHONY: vmdk
vmdk: $(BUILD)/master.vmdk

test: $(IMAGES)

.PHONY: clean
clean:
	@rm -rf $(BUILD)