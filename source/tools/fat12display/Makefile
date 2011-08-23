GCC = gcc
LD  = gcc
CFLAGS = -g
LFLAGS =

all:  fat12display

fat12display: fat12display.o
	@echo [LD] $@
	@$(LD) $(LFLAGS) -o $@ fat12display.o

.c.o:
	@echo [CC] $<
	@$(CC) $(CFLAGS) -c $< -o $@

clean:
	@echo [RM]
	@$(RM) fat12display.o fat12display
