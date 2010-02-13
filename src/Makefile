.PHONY: boot kernel
all: boot kernel

clean:
	@echo [Clean]
	@cd ./boot ; make clean
	@cd ..
	@cd ./kernel ; make clean

boot:
	@echo [Boot]
	@cd ./boot ; make 
kernel:
	@echo [Kernel]
	@cd ./kernel ; make 
