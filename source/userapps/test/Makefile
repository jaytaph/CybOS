all:
	gcc -c test1.c -fno-builtin
	gcc -c test2.c -fno-builtin
	gcc -c test3.c -fno-builtin
	gcc -c test4.c -fno-builtin
	nasm -f elf -o crt0.o crt0.S
	gcc -T cybos.ld -o test1.bin crt0.o test1.o -nostdlib -nostartfiles
	gcc -T cybos.ld -o test2.bin crt0.o test2.o -nostdlib -nostartfiles
	gcc -T cybos.ld -o test3.bin crt0.o test3.o -nostdlib -nostartfiles
	gcc -T cybos.ld -o test4.bin crt0.o test4.o -nostdlib -nostartfiles
	cp test1.bin ../tofloppy
	cp test2.bin ../tofloppy
	cp test3.bin ../tofloppy
	cp test4.bin ../tofloppy
