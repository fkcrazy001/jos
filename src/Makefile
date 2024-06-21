BUILD:=../build
SRC=.
ENTRYPOINT=0x10000
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
					$(BUILD)/lib/string.o \
					$(BUILD)/lib/vsprintf.o \
					$(BUILD)/lib/stdlib.o \

	$(shell mkdir -p $(dir $@))
	ld -m elf_i386 -static $^ -o $@ -Ttext $(ENTRYPOINT)

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
	dd if=$(BUILD)/system.bin of=$@ bs=512 count=200 seek=10 conv=notrunc


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

QEMU:= qemu-system-i386 \
	-m 32M \
	-boot c \
	-drive file=$(BUILD)/master.img,if=ide,index=0,media=disk,format=raw \
	-audiodev pa,id=hda \
	-machine pcspk-audiodev=hda \
	-rtc base=localtime \

.PHONY: qemu
qemu:$(BUILD)/master.img
	$(QEMU)


.PHONY: qemug
qemug:$(BUILD)/master.img
	$(QEMU) -s -S

$(BUILD)/master.vmdk:$(BUILD)/master.img
	qemu-img convert -pO vmdk $< $@
.PHONY: vmdk
vmdk: $(BUILD)/master.vmdk