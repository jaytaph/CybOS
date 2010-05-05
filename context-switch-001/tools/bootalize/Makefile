GCC = gcc
LD  = gcc
CFLAGS = -g

all:  fat12d

fat12d: fat12display.o 
	@echo [LD] $@
	@$(LD) -o $@ fat12display.o -lc 

bootalize: bootalize.o fat12.o
	@echo [LD] $@
	@$(LD) -o $@ bootalize.o fat12.o -lc -lmath

.c.o:
	@echo [CC] $<
	@$(CC) $(CFLAGS) -c $< -o $@

clean:
	@echo [RM]
	@$(RM) bootalize.o bootalize
	@$(RM) fat12.o fat12
	@$(RM) fat12display.o fat12d
