.SILENT :

GCC = gcc
LD  = gcc
AR  = ar
CFLAGS = -g

all:  libfat12

libfat12: fat12.o
	echo [AR] $<
	$(AR) rcs libfat12.a fat12.o

.c.o:
	echo [CC] $<
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	echo [RM]
	$(RM) fat12.o libfat12.a
