/******************************************************************************
 *
 *  File        : idt.c
 *  Description : Interrupt routines and managment.
 *
 *****************************************************************************/
#include "keyboard.h"
#include "general.h"
#include "service.h"
#include "console.h"
#include "kernel.h"
#include "kmem.h"
#include "errors.h"
#include "ktype.h"
#include "conio.h"
#include "timer.h"
#include "paging.h"
#include "pic.h"
#include "idt.h"
#include "gdt.h"
#include "io.h"


// Pointer to the kernel IDT
Uint64 *_kernel_idt;

// Default int handler
extern void handle_default_int (void);
extern void handle_syscall_int (void);

// IRQ handlers (defined in isr.S)
extern void handle_irq0 (void);
extern void handle_irq1 (void);
extern void handle_irq2 (void);
extern void handle_irq3 (void);
extern void handle_irq4 (void);
extern void handle_irq5 (void);
extern void handle_irq6 (void);
extern void handle_irq7 (void);
extern void handle_irq8 (void);
extern void handle_irq9 (void);
extern void handle_irq10 (void);
extern void handle_irq11 (void);
extern void handle_irq12 (void);
extern void handle_irq13 (void);
extern void handle_irq14 (void);
extern void handle_irq15 (void);

// Protected mode exception handlers (defined in isr.S)
extern void handle_exception0 (void);
extern void handle_exception1 (void);
extern void handle_exception2 (void);
extern void handle_exception3 (void);
extern void handle_exception4 (void);
extern void handle_exception5 (void);
extern void handle_exception6 (void);
extern void handle_exception7 (void);
extern void handle_exception8 (void);
extern void handle_exception9 (void);
extern void handle_exception10 (void);
extern void handle_exception11 (void);
extern void handle_exception12 (void);
extern void handle_exception13 (void);
extern void handle_exception14 (void);
extern void handle_exception15 (void);
extern void handle_exception16 (void);
extern void handle_exception17 (void);
extern void handle_exception18 (void);
extern void handle_exception19 (void);
extern void handle_exception20 (void);
extern void handle_exception21 (void);
extern void handle_exception22 (void);
extern void handle_exception23 (void);
extern void handle_exception24 (void);
extern void handle_exception25 (void);
extern void handle_exception26 (void);
extern void handle_exception27 (void);
extern void handle_exception28 (void);
extern void handle_exception29 (void);
extern void handle_exception30 (void);
extern void handle_exception31 (void);

// Array with handlers. Only reason is to make our life easier. We can use a simple
// for-loop to initialize all handlers.
Uint32 irq_handlers[16] = { (Uint32)&handle_irq0,  (Uint32)&handle_irq1,
                            (Uint32)&handle_irq2,  (Uint32)&handle_irq3,
                            (Uint32)&handle_irq4,  (Uint32)&handle_irq5,
                            (Uint32)&handle_irq6,  (Uint32)&handle_irq7,
                            (Uint32)&handle_irq8,  (Uint32)&handle_irq9,
                            (Uint32)&handle_irq10, (Uint32)&handle_irq11,
                            (Uint32)&handle_irq12, (Uint32)&handle_irq13,
                            (Uint32)&handle_irq14, (Uint32)&handle_irq15
                           };

// The first 32 interrupts are actually exceptions in protected mode. We handle them differently.
Uint32 exception_handlers[32] = { (Uint32)&handle_exception0,  (Uint32)&handle_exception1,
                                  (Uint32)&handle_exception2,  (Uint32)&handle_exception3,
                                  (Uint32)&handle_exception4,  (Uint32)&handle_exception5,
                                  (Uint32)&handle_exception6,  (Uint32)&handle_exception7,
                                  (Uint32)&handle_exception8,  (Uint32)&handle_exception9,
                                  (Uint32)&handle_exception10, (Uint32)&handle_exception11,
                                  (Uint32)&handle_exception12, (Uint32)&handle_exception13,
                                  (Uint32)&handle_exception14, (Uint32)&handle_exception15,
                                  (Uint32)&handle_exception16, (Uint32)&handle_exception17,
                                  (Uint32)&handle_exception18, (Uint32)&handle_exception19,
                                  (Uint32)&handle_exception20, (Uint32)&handle_exception21,
                                  (Uint32)&handle_exception22, (Uint32)&handle_exception23,
                                  (Uint32)&handle_exception24, (Uint32)&handle_exception25,
                                  (Uint32)&handle_exception26, (Uint32)&handle_exception27,
                                  (Uint32)&handle_exception28, (Uint32)&handle_exception29,
                                  (Uint32)&handle_exception30, (Uint32)&handle_exception31
                                 };


  // Structure for IDTR.
  #pragma pack(1)
  typedef struct { Uint16 limit; Uint32 base; } TIDTR;



int disable_ints (void) {
  Uint32 flags;
  int state;

  // Get flags and see if interrupt flag is set
  __asm__ __volatile__ (" pushfl ;  pop %%eax" : "=a"(flags));
  state = flags & 0x200;

  cli ();

  return state;
}


void restore_ints (int state) {
  // Only set interrupts back on when state != 0
  if (state != 0) sti ();
}

// =================================================================================
void idt_set_descriptor (int index, Uint64 descriptor) {
  _kernel_idt[index] = descriptor;
}

// =================================================================================
Uint64 idt_get_descriptor (int index) {
	return _kernel_idt[index];
}


/********************************************************
 * Sets up the Interrupt Descriptor Table.
 */
int idt_init (void) {
  Uint64 idt;
  TIDTR idtr;
  int i;
  Uint32 phys_idt_addr;

  // Allocate room for kernel IDT (for 256 descriptors) and save it's physical addres
  _kernel_idt = kmalloc_physical (256*8, &phys_idt_addr);


  // Assign all interrupts to the default interrupt handler.
  for (i=0; i!=256; i++) {
    idt = idt_create_descriptor ((Uint32)&handle_default_int, SEL(KERNEL_CODE_DESCR, TI_GDT+RPL_RING0), IDT_PRESENT+IDT_DPL0+IDT_INTERRUPT_GATE);
// TODO: Remove me. This helps us debugging when bochs handle these interrupts instead of our OS
    if (i == 6 || i == 8 || i == 13 || i == 14) continue;
    idt_set_descriptor (i, idt);
  }

  // Add standard IRQ handlers (0x50..0x5F)
  for (i=0; i!=16; i++)  {
    idt = idt_create_descriptor (irq_handlers[i], SEL(KERNEL_CODE_DESCR, TI_GDT+RPL_RING0), IDT_PRESENT+IDT_DPL0+IDT_INTERRUPT_GATE);
    idt_set_descriptor (IRQINT_START+i, idt);
  }

  // Add exception handlers (0..31)
  for (i=0; i!=32; i++) {
    idt = idt_create_descriptor (exception_handlers[i], SEL(KERNEL_CODE_DESCR, TI_GDT+RPL_RING0), IDT_PRESENT+IDT_DPL0+IDT_INTERRUPT_GATE);
// TODO: Remove me. This helps us debugging when bochs handle these interrupts instead of our OS
    if (i == 6 || i == 8 || i == 13 || i == 14) continue;
    idt_set_descriptor (i, idt);
  }

  // Add syscall interrupt
  idt = idt_create_descriptor ((Uint32)&handle_syscall_int, SEL(KERNEL_CODE_DESCR, TI_GDT+RPL_RING0), IDT_PRESENT+IDT_DPL3+IDT_TRAP_GATE);
  idt_set_descriptor (SYSCALL_INT, idt);

  // Setup idtr structure
  idtr.limit = (256*8)-1;
  idtr.base  = phys_idt_addr;  // IDT has to be loaded through the physical address (?)
  idtr.base += 0xC0000000;

  // Disable all IRQ's
  pic_mask_irq (0x1111);

  // Load new interrupt descriptor table
  __asm__ __volatile__ ("lidt (%0)" : :"p" (&idtr));

  // Enable all IRQ's
  pic_mask_irq (0x0000);

  return ERR_OK;
}



// =================================================================================
// Same as the exception and syscall handler, it could have been handled by a lowlevel
// lookup-table, but somehow this looks more readable to me.
void do_handle_irq (TREGS *r) {
  int rescheduling = 0;

  switch (r->int_no) {
    case 0 :
             // CS is selector, first 2 bits is the RPL so the timer function knows
             // in which context it is being called.
             rescheduling = timer_interrupt (r->cs & 0x3);
             break;
    case 1 :
             rescheduling = keyboard_interrupt ();
             break;
    case 2 :
             break;
    case 3 :
             break;
    case 4 :
             break;
    case 5 :
             break;
    case 6 :
             break;
    case 7 :
             break;
    case 8 :
             break;
    case 9 :
             break;
    case 10 :
              break;
    case 11 :
              break;
    case 12 :
              break;
    case 13 :
              break;
    case 14 :
              break;
    case 15 :
              break;
    default :
              break;
  }

  // Acknowledge IRQ. Needed because can get rescheduled now
  outb (0x20, 0x20);
  outb (0xA0, 0x20);

  // Reschedule if needed
  if (rescheduling) reschedule();
}

// =================================================================================
// Note that we have a POINTER to the regs, not the actual regs itself. This is
// done because wel CALL the do_handle_syscall instead of jumping to it. This means
// the call automatically places the ret addres onto the stack and that messes up the
// rest of the parameters. Therefor, we just send a pointer.
int do_handle_syscall (TREGS *r) {
  return service_interrupt (r->eax & 0x0000FFFF, r->ebx, r->ecx, r->edx, r->esi, r->edi);
}

// =================================================================================
void  do_handle_default_int (TREGS r) {
  // Do nothing
}

// =================================================================================
void do_handle_exception (TREGS  *r) {
  kprintf ("Handling exception %d...\n", r->int_no);

  // Debug info
  print_cpu_info (r);

  switch (r->int_no) {
    case 0 : break;
    case 1 : break;
    case 2 : break;
    case 3 : break;
    case 4 : break;
    case 5 : break;
    case 6 : break;
    case 7 : break;
    case 8 : break;
    case 9 : break;
    case 10 : break;
    case 11 : break;
    case 12 : break;
    case 13 :
              kprintf ("General protection fault.\n");
              print_cpu_info (r);
              kdeadlock ();
              break;
    case 14 :
              do_page_fault (r);
              break;
    case 15 : break;
    case 16 : break;
    case 17 : break;
    case 18 : break;
    case 19 : break;
    case 20 : break;
    case 21 : break;
    case 22 : break;
    case 23 : break;
    case 24 : break;
    case 25 : break;
    case 26 : break;
    case 27 : break;
    case 28 : break;
    case 29 : break;
    case 30 : break;
    case 31 : break;
    case 32 : break;
    default :
              kdeadlock ();
              break;
  }
}


// =================================================================================
void print_cpu_info (TREGS *r) {
  int reg_cr0,reg_cr2,reg_cr3;
  int reg_ds, reg_es, reg_fs, reg_gs;

  __asm__ __volatile__ (
    "mov %%cr0, %0 \n\t"
    "mov %%cr2, %1 \n\t"
    "mov %%cr3, %2 \n\t"
    : "=r" (reg_cr0), "=r" (reg_cr2), "=r" (reg_cr3)
  );

  __asm__ __volatile__ (
    "mov %%ds, %0 \n\t"
    "mov %%es, %1 \n\t"
    "mov %%fs, %2 \n\t"
    "mov %%gs, %3 \n\t"
    : "=r" (reg_ds), "=r" (reg_es), "=r" (reg_fs), "=r" (reg_gs)
  );

  kprintf ("CPU STATE\n\n");
  kprintf ("EAX=%08X  EBX=%08X\n", r->eax, r->ebx);
  kprintf ("ECX=%08X  EDX=%08X\n", r->ecx, r->edx);
  kprintf ("ESP=%08X  EBP=%08X\n", r->useresp, r->ebp);
  kprintf ("ESI=%08X  EDI=%08X\n", r->esi, r->edi);
  kprintf ("IOPL=? id vip vif ac vm rf nt of df if tf sf zf af pf cf (%08X)\n", r->eflags);
  kprintf ("SEG selector     base     limit G D\n");
  kprintf ("SEG sltr(index|ti|rpl)    base    limit G D\n");
  kprintf (" CS:%04X (%04X |%c |%d) 00000000 00000000 0 0\n", r->cs,  (r->cs>>3),  (r->cs >>2)&1?'L':'G', ((r->cs )&3));
  kprintf (" DS:%04X (%04X |%c |%d) 00000000 00000000 0 0\n", reg_ds, (reg_ds>>3), (reg_ds>>2)&1?'L':'G', ((reg_ds)&3));
  kprintf (" SS:%04X (%04X |%c |%d) 00000000 00000000 0 0\n", r->ss,  (r->ss>>3),  (r->ss >>2)&1?'L':'G', ((r->ss )&3));
  kprintf (" ES:%04X (%04X |%c |%d) 00000000 00000000 0 0\n", reg_es, (reg_es>>3), (reg_es>>2)&1?'L':'G', ((reg_es)&3));
  kprintf (" FS:%04X (%04X |%c |%d) 00000000 00000000 0 0\n", reg_fs, (reg_fs>>3), (reg_fs>>2)&1?'L':'G', ((reg_fs)&3));
  kprintf (" GS:%04X (%04X |%c |%d) 00000000 00000000 0 0\n", reg_gs, (reg_gs>>3), (reg_gs>>2)&1?'L':'G', ((reg_gs)&3));
  kprintf (" RIP=%08X\n", r->eip);
  kprintf (" CR0=0x%08X CR1=0x? CR2=0x%08X\n", reg_cr0, reg_cr2);
  kprintf (" CR3=0x%08X\n", reg_cr3);
}

