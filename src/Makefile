.SILENT :

.PHONY: stdlib kernel boot

all: stdlib kernel boot

clean:
	echo [Clean]
	make -C boot clean
	make -C kernel clean

stdlib:
	echo [StdLib]
	make -C stdlib

boot:
	echo [Boot]
	make -C stdlib

kernel:
	echo [Kernel]
	make -C kernel
