#
# $Id$
#
#

# Programs
AS = nasm
RM = rm

BOOT_TARGET = boot.bin
LOADER_TARGET = loader.bin

all: boot loader install

boot:
	@echo [AS] boot
	@$(AS) -w-orphan-labels -o boot.bin boot.S

loader:
	@echo [AS] loader
	@$(AS) -w-orphan-labels -o loader.bin loader.S

# Install
install:
	@echo [Create bootdisk]
	@dd if=/dev/zero of=../floppya.dat bs=1024 count=1024 >/dev/null 2>&1
	@dd if=boot.bin of=../floppya.dat bs=512 seek=0 conv=notrunc >/dev/null 2>&1
	@dd if=loader.bin of=../floppya.dat bs=512 seek=1 conv=notrunc >/dev/null 2>&1

# Clean up
clean:
	@echo [RM]
	$(RM) $(LOADER_TARGET)
	$(RM) $(BOOT_TARGET)
