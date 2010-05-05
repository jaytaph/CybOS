# $header$

# Standard compiler programs
CC       = gcc
LD       = ld
AS       = nasm
OBJCOPY  = objcopy
RM       = rm
AR       = ar

# Compiler flags
CC_FLAGS = -nostdlib -nostdinc -fno-builtin -fno-exceptions -fomit-frame-pointer -Wall
INCLUDES = -I. -I.. -I../stdinc -Iinc -Istdio -Istring

CFLAGS = $(CC_FLAGS) $(INCLUDES) $(USR_FLAGS)

# Default target
TARGET = stdlib.a

# Object definitions
OBJS =	stdio/stdio.o \
        stdio/sprintf.o \
        stdio/vprintf.o \
        stdio/_printf.o \
        stdio/printf.o \
        stdio/vsprintf.o \
        string/strlen.o \
        string/strncpy.o \
        string/strncmp.o \
        string/strcmp.o \
        string/memcpy.o \
        string/memset.o \
        string/movedata.o

# Default target
all: stdlib install

#
stdlib: $(OBJS)
	@echo [AR] $<
	$(AR) -r $(TARGET) $(OBJS)

# Clean up all makeable files
clean:
	@echo [RM]
	$(RM) $(OBJS)
	$(RM) $(TARGET)

# Implicit rules
.c.o:
	@echo [CC] $<
	@$(CC) $(CFLAGS)  -c $< -o $@


# Install does nothing particulair
install:
