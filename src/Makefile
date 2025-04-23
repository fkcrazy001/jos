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
					$(BUILD)/kernel/keyboard.o \
					$(BUILD)/kernel/arena.o \
					$(BUILD)/kernel/ide.o \
					$(BUILD)/kernel/device.o \
					$(BUILD)/kernel/buffer.o \
					$(BUILD)/kernel/system.o \
					$(BUILD)/fs/super.o \
					$(BUILD)/fs/bmap.o \
					$(BUILD)/fs/inode.o \
					$(BUILD)/fs/namei.o \
					$(BUILD)/fs/file.o \
					$(BUILD)/lib/string.o \
					$(BUILD)/lib/vsprintf.o \
					$(BUILD)/lib/stdlib.o \
					$(BUILD)/lib/bitmap.o \
					$(BUILD)/lib/syscall.o \
					$(BUILD)/lib/list.o \
					$(BUILD)/lib/fifo.o \
					$(BUILD)/lib/printf.o \
					$(BUILD)/builtin/osh.o \


	$(shell mkdir -p $(dir $@))
	ld  ${LDFLAGS} $^ -o $@

$(BUILD)/system.bin: $(BUILD)/kernel.bin

	objcopy -O binary $< $@

$(BUILD)/system.map: $(BUILD)/kernel.bin
	nm $< | sort > $@

include utils/image.mk
include utils/cmd.mk
include utils/cdrom.mk