.SILENT :

.PHONY: kernel boot

all: kernel boot install

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

install:
	echo [Create bootdisk]
	sudo ./bootalize ../fat12.img bootlvl1.bin bootlvl2.bin

