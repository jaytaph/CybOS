# $header$

.PHONY: floppy install

# Standard compiler programs
CC       = gcc
LD       = ld
AS       = nasm
OBJCOPY  = objcopy
RM       = rm

# Linker flags
LD_ENTRY  = -e kernel_entry
LD_TEXT   = -Ttext=0xC0010000 -Map kernel.map

#LIBS =  ../stdlib/stdlib.a
LIBS =


# Main target
TARGET = kernel.bin


# Compiler and user flags
CC_FLAGS = -nostdlib -nostdinc -fno-builtin -fno-exceptions -fomit-frame-pointer -Wall

USR_FLAGS = -D__KERNEL__ -D__DEBUG__

#INCLUDES = -I. -I.. -I../stdinc -Iinc
INCLUDES = -I. -I..  -Iinc

CFLAGS = $(CC_FLAGS) $(INCLUDES) $(USR_FLAGS)


# Objects
OBJS =  main.o \
        kernel.o \
        console.o \
        conio.o \
        kmem.o \
        heap.o \
        page.o \
        paging.o \
        gdt.o \
        idt.o \
        isr.o \
        schedule.o \
        tss.o \
        timer.o \
        keyboard.o \
        service.o \
        command.o \
        io.o \
        pit.o \
        pic.o \
        mutex.o \
        mutexasm.o \
        klib.o


# Default target
all: kernel.bin install

# Cleans up the kernel objects
clean:
	@echo [RM]
	@$(RM) *.o
	@$(RM) $(TARGET)
	@$(RM) *.map


kernel.bin: kernel.out
	@echo [OC] $@
	$(OBJCOPY) -O binary kernel.out kernel.bin

# Kernel binary
kernel.out: $(OBJS)
	@echo [LD] $@
	@$(LD) $(LD_TEXT) $(LD_ENTRY) -s -o $@ $(OBJS) $(LIBS)

# Implicit Rules
.c.o:
	@echo [CC] $<
	@$(CC) $(CFLAGS) -c -o $@ $<
.S.o:
	@echo [AS] $<
	@$(AS) -f elf -o $@ $<


# Install the kernel
install:
	@echo Installing...
	@dd if=kernel.bin of=../floppya.dat bs=512 seek=3 conv=notrunc >/dev/null 2>&1



floppy:
	dd if=../boot/boot.bin   of=../fd.img bs=512 seek=0 conv=notrunc >/dev/null 2>&1
	dd if=../boot/loader.bin of=../fd.img bs=512 seek=1 conv=notrunc >/dev/null 2>&1
	dd if=kernel.bin         of=../fd.img bs=512 seek=3 conv=notrunc >/dev/null 2>&1
	dd if=../fd.img of=/dev/fd0 bs=512 >/dev/null 2>&1
