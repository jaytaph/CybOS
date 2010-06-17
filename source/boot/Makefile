#
# $Id$
#
#

# Programs

.SILENT : 

AS = nasm
RM = rm

BOOTLVL1_TARGET = ../bootlvl1.bin
BOOTLVL2_TARGET = ../bootlvl2.bin

all: bootlvl1 bootlvl2 

bootlvl1:
	echo [AS] bootlvl1
	$(AS) -w-orphan-labels -o $(BOOTLVL1_TARGET) -I level1 level1/bootlvl1.S

bootlvl2:
	echo [AS] bootlvl2
	$(AS) -w-orphan-labels -o $(BOOTLVL2_TARGET) -I level2 level2/bootlvl2.S

# Clean up
clean:
	echo [RM]
	$(RM) $(BOOTLVL1_TARGET) || true
	$(RM) $(BOOTLVL2_TARGET) || true
