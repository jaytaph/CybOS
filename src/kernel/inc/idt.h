/******************************************************************************
 *
 *  File        : idt.h
 *  Description : Interrupt routines and managment headers
 *
 *****************************************************************************/
#ifndef __IDT_H__
#define __IDT_H__


  // IRQ to INT mappings. Must move them to away to at least 0x20 so they won't
  // interfere with pmode exceptions. We place them at 0x50 so they are out of the way.
  #define TIMER_IRQ             0x50
  #define KEYBOARD_IRQ          0x51
  #define CASCADE_IRQ           0x52
  #define SERIALPORT1_IRQ       0x53
  #define SERIALPORT2_IRQ       0x54
  #define PARALLELPORT2_IRQ     0x55
  #define DISKETTE_IRQ          0x56
  #define PARALLELPORT1_IRQ     0x57

  #define REALTIMECLOCK_IRQ     0x58
  #define CGARETRACE_IRQ        0x59
  #define RESERVED1_IRQ         0x5A
  #define RESERVED2_IRQ         0x5B
  #define PS2_IRQ               0x5C
  #define FPU_IRQ               0x5D
  #define HARDDISK_IRQ          0x5E
  #define RESERVED3_IRQ         0x5F

  #define IRQINT_START          TIMER_IRQ     // Start of irq int. Note that low irq (0..7) and high irq's (8..15) should be sequential. Don't place a gap in between.


  // System call interrupt. Our main service routine
  #define	SYSCALL_INT	          0x42            // 42, since it's the answer to the Ultimate Question of Life, the Universe, and Everything
  #define	SYSCALL_INT_STR      "0x42"





  // Register structure as found on the stack when calling the hi level interrupt handlers.
  // This way we can use them as function arguments.
  typedef struct regs {
    Uint32 dum1, dum2, dum3, dum4, dum5;             // Old segment registers
    Uint32 edi, esi, ebp, esp, ebx, edx, ecx, eax;   // Pushed by pusha.
    Uint32 int_no, err_code;                         // Interrupt number and error code (if applicable)
    Uint32 eip, cs, eflags, useresp, ss;             // Pushed by the processor automatically.
  } TREGS;


  // Interrupt defines
  #define hlt() __asm__ __volatile__ ("hlt");   // Halt processor
  #define cli() __asm__ __volatile__ ("cli");   // Clear interrupt flag
  #define sti() __asm__ __volatile__ ("sti");   // Set interrupt flag


  // Functions
  int idt_init (void);
  unsigned long long idt_create_descriptor (Uint32 offset, Uint16 selector, Uint8 flags);

  void print_cpu_info (TREGS *r);

#endif  // _IDT_H__
