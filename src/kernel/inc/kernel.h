/******************************************************************************
 *
 *  File        : kernel.h
 *  Description : Kernel main source header file.
 *
 *****************************************************************************/
#ifndef __KERNEL_H__
#define __KERNEL_H__

//  #include <stdarg.h>
  #include "kernel.h"
  #include "ktype.h"
  #include "klib.h"

  #include "general.h"
  #include "service.h"
  #include "gdt.h"
  #include "idt.h"
  #include "tss.h"
  #include "signal.h"

  // GDT Descriptors (fixed)
  #define NULL_DESCR              0x0     // Cannot use
  #define KERNEL_CODE_DESCR       0x1     // Global kernel code descriptor (whole memory range)
  #define KERNEL_DATA_DESCR       0x2     // Global kernel data descriptor (whole memory range)
  #define TSS_TASK_DESCR          0x3     // TSS descriptor for current running process
  #define LDT_TASK_DESCR          0x4     // LDT descriptor for current running process
  #define KUSER_CODE_DESCR        0x5     // Global user code descriptor (whole memory range)
  #define KUSER_DATA_DESCR        0x6     // Global user data descriptor (whole memory range)

  #define USER_CODE_DESCR         0x0     // Global code descriptor in the LDT (we can use descriptor 0 in LDT's)
  #define USER_DATA_DESCR         0x1     // Global data descriptor in the LDT
  #define USERTASK_LDT_SIZE         2     // Only 2 descriptors in every LDT table

  // Selectors made out of the TSS and LDT descriptors (these never change also)
  #define TSS_TASK_SEL    SEL(TSS_TASK_DESCR, TI_GDT+RPL_RING0)
  #define LDT_TASK_SEL    SEL(LDT_TASK_DESCR, TI_GDT+RPL_RING0)


  // This instruction does nothing, however, it triggers a breakpoint in the bochs debugger.
  // Very handy for testing purposes. You must have "magic_break: enabled=1" in your bochsrc
  #define BOCHS_BREAKPOINT      __asm__ __volatile__ ( "xchg %bx,%bx\n\t");

  extern char end;             // Defined by the linker. This is the end of the code cq start of the heap (sbrk)

  extern unsigned long long *_kernel_idt;       // Pointer to the IDT of the kernel
  extern unsigned long long *_kernel_gdt;       // Pointer to the GDT of the kernel

  extern unsigned long long _kernel_ticks;      // Number of ticks that the kernel is running
  extern TCONSOLE *_kconsole;                   // TConsole for kernel info (first screen)

  // Kernel Entry Point
  void kernel_entry (int stack_start, int total_sys_memory);

  // Kernel Deadlock, no return
  void kdeadlock (void);

  // Kernel Panic message, no return
  void kpanic (const char *fmt, ...);

  // Print on kernel_console
  void kprintf (const char *fmt, ...);

  // The Construct Console Application
  void construct (void);



#endif //__KERNEL_H__
