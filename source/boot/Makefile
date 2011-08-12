#
# $Id$
#
#

# Programs

.SILENT : 

AS = nasm
RM = rm

AS_FLAGS=-w-orphan-labels 

BOOTLVL1_TARGET = ../../binary/boot/bootlvl1.bin
BOOTLVL2_TARGET = ../../binary/boot/bootlvl2.bin

all: bootlvl1 bootlvl2 

bootlvl1:
	echo [AS] bootlvl1
	$(AS) $(AS_FLAGS) -o $(BOOTLVL1_TARGET) -I level1 level1/bootlvl1.S

bootlvl2:
	echo [AS] bootlvl2
	$(AS) $(AS_FLAGS) -o $(BOOTLVL2_TARGET) -I level2 level2/bootlvl2.S

# Clean up
clean:
	echo [RM]
	$(RM) $(BOOTLVL1_TARGET) || true
	$(RM) $(BOOTLVL2_TARGET) || true
