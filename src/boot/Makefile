#
# $Id$
#
#

# Programs

.SILENT : 

AS = nasm
RM = rm

BOOTLVL1_TARGET = bootlvl1.bin
BOOTLVL2_TARGET = bootlvl2.bin

all: bootlvl1 bootlvl2 install

bootlvl1:
	echo [AS] bootlvl1
	$(AS) -w-orphan-labels -o bootlvl1.bin -I level1 level1/bootlvl1.S

bootlvl2:
	echo [AS] bootlvl2
	$(AS) -w-orphan-labels -o bootlvl2.bin -I level2 level2/bootlvl2.S

# Install
install:
	echo [Create bootdisk]
	./bootalize ../../fat12.img bootlvl1.bin bootlvl2.bin
	#dd if=boot.bin of=../../fat12.img bs=512 seek=0 conv=notrunc 

# Clean up
clean:
	echo [RM]
	$(RM) $(BOOTLVL1_TARGET)
	$(RM) $(BOOTLVL2_TARGET)
