BUILD:=../build
SRC=.
MULTIBOOT2:=0x10000
ENTRYPOINT=$(shell python -c "print(f'0x{$(MULTIBOOT2)+64:x}')")
CFLAGS:=-m32
DEBUG:=-g
INCLUDE:=-I$(SRC)/include

include config.mk

CFLAGS+=-fno-builtin # no build in funcs
CFLAGS+=-nostdinc  # no std include
CFLAGS+=-fno-pic    # no position independent code
CFLAGS+=-fno-pie   # no position independent exec
CFLAGS+=-nostdlib
CFLAGS+=-fno-stack-protector
CFLAGS:=$(strip ${CFLAGS})

LDFLAGS := -m elf_i386 \
			-static \
			-Ttext $(ENTRYPOINT) \
			--section-start=.multiboot2=$(MULTIBOOT2)
LDFLAGS:=$(strip ${LDFLAGS})

$(BUILD)/boot/%.bin: $(SRC)/boot/%.asm
	$(shell mkdir -p $(dir $@))
	nasm -f bin $< -o $@

$(BUILD)/%.o: $(SRC)/%.asm
	$(shell mkdir -p $(dir $@))
	nasm -f elf32 $(DEBUG) $< -o $@

$(BUILD)/%.o: $(SRC)/%.c
	$(shell mkdir -p $(dir $@))
	gcc $(CFLAGS) $(DEBUG) $(INCLUDE) -c $< -o $@

$(BUILD)/kernel.bin: $(BUILD)/kernel/start.o \
					$(BUILD)/kernel/main.o \
					$(BUILD)/kernel/io.o \
					$(BUILD)/kernel/printk.o \
					$(BUILD)/kernel/console.o \
					$(BUILD)/kernel/assert.o \
					$(BUILD)/kernel/debug.o \
					$(BUILD)/kernel/global.o \
					$(BUILD)/kernel/task.o \
					$(BUILD)/kernel/scheduler.o \
					$(BUILD)/kernel/interrupt.o \
					$(BUILD)/kernel/handler.o \
					$(BUILD)/kernel/pic.o \
					$(BUILD)/kernel/clock.o \
					$(BUILD)/kernel/time.o \
					$(BUILD)/kernel/rtc.o \
					$(BUILD)/kernel/memory.o \
					$(BUILD)/kernel/gate.o \
					$(BUILD)/kernel/thread.o \
					$(BUILD)/kernel/mutex.o \
					$(BUILD)/lib/string.o \
					$(BUILD)/lib/vsprintf.o \
					$(BUILD)/lib/stdlib.o \
					$(BUILD)/lib/bitmap.o \
					$(BUILD)/lib/syscall.o \
					$(BUILD)/lib/list.o \

	$(shell mkdir -p $(dir $@))
	ld  ${LDFLAGS} $^ -o $@

$(BUILD)/system.bin: $(BUILD)/kernel.bin

	objcopy -O binary $< $@

$(BUILD)/system.map: $(BUILD)/kernel.bin
	nm $< | sort > $@

$(BUILD)/master.img: $(BUILD)/boot/boot.bin \
					$(BUILD)/boot/loader.bin \
					$(BUILD)/system.bin \
					$(BUILD)/system.map \
					
	yes | bximage -q -hd=16 -func=create -sectsize=512 -imgmode=flat $@
	dd if=$(BUILD)/boot/boot.bin of=$@ bs=512 count=1 conv=notrunc
	dd if=$(BUILD)/boot/loader.bin of=$@ bs=512 count=4 seek=2 conv=notrunc
# system.bin需要小于100k，否则  count = size / 0.5k 就放不下了
	test -n "$$(find $(BUILD)/system.bin -size -100k)"
	dd if=$(BUILD)/system.bin of=$@ bs=512 count=200 seek=10 conv=notrunc

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


test:$(BUILD)/master.img

.PHONY: clean
clean:
	@rm -rf $(BUILD)

.PHONY: bochs
bochs:$(BUILD)/master.img
	bochs -q -f ../bochs/bochsrc

.PHONY: bochsg
bochsg:$(BUILD)/master.img
	bochs-gdb -q -f ../bochs/bochsrc.gdb

.PHONY: bochsb
bochsb:$(BUILD)/kernel.iso
	bochs -q -f ../bochs/bochsrc.grub -unlock

QEMU:= qemu-system-i386 \
	-m 32M \
	-audiodev pa,id=hda \
	-machine pcspk-audiodev=hda \
	-rtc base=localtime \

QEMU_DISK:=-boot c \
	-drive file=$(BUILD)/master.img,if=ide,index=0,media=disk,format=raw \

QEMU_CDROM:=-boot d \
	-drive file=$(BUILD)/kernel.iso,media=cdrom \

QEMU_DEBUG:= -s -S

.PHONY: qemu
qemu:$(BUILD)/master.img
	$(QEMU) $(QEMU_DISK)

.PHONY: qemug
qemug:$(BUILD)/master.img
	$(QEMU) $(QEMU_DISK) $(QEMU_DEBUG)

.PHONY: qemub
qemub:$(BUILD)/kernel.iso
	$(QEMU) $(QEMU_CDROM) \
	# $(QEMU_DEBUG)

$(BUILD)/master.vmdk:$(BUILD)/master.img
	qemu-img convert -pO vmdk $< $@
.PHONY: vmdk
vmdk: $(BUILD)/master.vmdk