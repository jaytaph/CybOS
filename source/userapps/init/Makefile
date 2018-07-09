all:
	gcc -c init.c -fno-builtin -m32
	nasm -f elf32 -o crt0.o crt0.S
	ld -T cybos.ld -melf_i386 -o init.bin crt0.o init.o -nostdlib -nostartfiles
	cp init.bin ../../../binary/apps/init.bin
