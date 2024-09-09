
$(BUILD)/master.img: $(BUILD)/boot/boot.bin \
					$(BUILD)/boot/loader.bin \
					$(BUILD)/system.bin \
					$(BUILD)/system.map \
					$(SRC)/utils/master.sfdisk \
					
	yes | bximage -q -hd=16 -func=create -sectsize=512 -imgmode=flat $@
	dd if=$(BUILD)/boot/boot.bin of=$@ bs=512 count=1 conv=notrunc
	dd if=$(BUILD)/boot/loader.bin of=$@ bs=512 count=4 seek=2 conv=notrunc
# system.bin需要小于100k，否则  count = size / 0.5k 就放不下了
	test -n "$$(find $(BUILD)/system.bin -size -100k)"
	dd if=$(BUILD)/system.bin of=$@ bs=512 count=200 seek=10 conv=notrunc

# 使用分区
	sfdisk $@ < $(SRC)/utils/master.sfdisk

# 制作文件系统
	sudo losetup /dev/loop0 --partscan $@

	sudo mkfs.minix -1 -n 14 /dev/loop0p1

	sudo mount /dev/loop0p1 /mnt

	sudo chown ${USER} /mnt

	mkdir -p /mnt/home
	mkdir -p /mnt/d1/d2/d3/d4

	echo "hello jos, from root dir file..." > /mnt/hello.txt
	echo "hello jos, from home dir file..." > /mnt/home/hello.txt

	sudo umount /mnt

	sudo losetup -d /dev/loop0

$(BUILD)/slave.img:
	yes | bximage -q -hd=32 -func=create -sectsize=512 -imgmode=flat $@
# 开始分区
	sfdisk $@ < $(SRC)/utils/slave.sfdisk
	sudo losetup /dev/loop0 --partscan $@

	sudo mkfs.minix -1 -n 14 /dev/loop0p1

	sudo mount /dev/loop0p1 /mnt

	sudo chown ${USER} /mnt

	echo "hello jos, from slave root dir file..." > /mnt/hello.txt

	sudo umount /mnt

	sudo losetup -d /dev/loop0

IMAGES:= $(BUILD)/master.img $(BUILD)/slave.img

image: $(IMAGES)

.PHONY: mount0
mount0: $(BUILD)/master.img
	sudo losetup /dev/loop0 --partscan $<
	sudo mount /dev/loop0p1 /mnt
	sudo chown ${USER} /mnt

.PHONY: umount0
umount0: /dev/loop0
	sudo umount /mnt
	sudo losetup -d $<

.PHONY: mount1
mount1: $(BUILD)/slave.img
	sudo losetup /dev/loop1 --partscan $<
	sudo mount /dev/loop1p1 /mnt
	sudo chown ${USER} /mnt

.PHONY: umount1
umount1: /dev/loop1
	sudo umount /mnt
	sudo losetup -d $<