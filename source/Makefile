.SILENT :

.PHONY: kernel boot apps

all: kernel boot apps install

clean:
	echo [Clean]
	make -C boot clean
	make -C kernel clean

boot:
	echo [Boot]
	make -C boot

kernel:
	echo [Kernel]
	make -C kernel

apps:
	echo [Apps]
	make -C userapps

install:
	echo [Create bootdisk]
	sudo ./bootalize ../binary/image/fat12.img ../binary/boot/bootlvl1.bin ../binary/boot/bootlvl2.bin

