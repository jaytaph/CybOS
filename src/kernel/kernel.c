/******************************************************************************
 *
 *  File        : kernel.c
 *  Description : Kernel main source file.
 *
 *****************************************************************************/

#include "command.h"
#include "console.h"
#include "version.h"
#include "kernel.h"
#include "klib.h"
#include "paging.h"
#include "ktype.h"
#include "schedule.h"
#include "conio.h"
#include "timer.h"
#include "kmem.h"
#include "heap.h"
#include "service.h"
#include "gdt.h"
#include "idt.h"
#include "pit.h"
#include "pic.h"
#include "io.h"

  // 64bit tick counter (we assume this OS will have decent uptimes :) )
  unsigned long long _kernel_ticks;

  CYBOS_TASK construct_task;           // Construct() task

  void tprintf (const char *fmt, ...);


  CYBOS_TASK task1;                    // Dummy user task 1. Remove later.
  void task_1 (void);                  // Dummy Test task 1. Remove later.
  CYBOS_TASK task2;                    // Dummy user task 2. Remove later.
  void task_2 (void);                  // Dummy Test task 2. Remove later.


/****************************************************************************
 * Startup of the kernel.
 */
void kernel_entry (int stack_start, int total_sys_memory) {
  // Number of times the timer-interrupts is called.
  _kernel_ticks = 0;

  // Initialise memory and console first
  kmem_init (total_sys_memory);
  console_init ();

  // Show some welcome messages
  kprintf ("\n");
  kprintf ("Initialising CybOS kernel v%s.%s (%s)\n", KERNEL_VERSION_MAJOR, KERNEL_VERSION_MINOR, KERNEL_COMPILER);
  kprintf ("This kernel was compiled at %s on %s\n", KERNEL_COMPILE_TIME, KERNEL_COMPILE_DATE);
  kprintf ("Available system memory: %dKB (%dMB)\n", total_sys_memory / 1024, total_sys_memory / (1024*1024));
  kprintf ("\n");

  kprintf ("Initializing kernel components: [ ");

  kprintf ("GDT ");
  gdt_init ();

  // Initialize PIC interrupt controller
  kprintf ("PIC ");
  pic_init();

  // Initialize interrupt timer
  kprintf ("PIT ");
  pit_set_frequency (0x100);

  // Create interrupt and exception handlers
  kprintf ("IDT ");
//  idt_init();

  // Setup paging
  kprintf ("PAG ");
  paging_init();

  kprintf ("MEM ");
  heap_init();

  // Everything is initialized for the kernel. TODO: why not copy the stack?
  _current_pagedirectory = clone_pagedirectory (_kernel_pagedirectory);
  set_pagedirectory (_current_pagedirectory);

  kprintf ("STK ");
  stack_init (stack_start);

  // Initialize multitasking environment
  kprintf ("TSK ");
  sched_init ();

  kprintf ("]\n");
  kprintf ("Kernel initialization done. Unable to free %d bytes.\nTransfering control to user mode.\n", _unfreeable_kmem);

  // Switch to ring3 and continue as being the idle-task
  switch_to_usermode ();
  for (;;) ;

  // Set the current_task. This will tell the scheduler to start task-switching
  _current_task = _task_list;


  int pid = sys_fork ();
  if (pid == 0) {
    kprintf ("PID[%05d] After first fork() : Child says %d\n", _current_task->pid, pid);

    strncpy (_current_task->name, "Test process 1\0", 15);

    while (1) {
/*
      // Just something that takes a long time so we can see if UTIME is actually moving
      int i,j,x,y,z;
      for (i=0; i!=1000; i++) {
        for (j=0; j!=1000; j++) {
          x = i * j;
          y = i * x;
          z = x / 1241;
        }
      }
*/
      sys_sleep (5000);

      kprintf ("\n\n");
      kprintf ("PID  PPID TASK                 STAT  PRIO  KTIME     UTIME\n", _current_task->pid);
      CYBOS_TASK *t;
      for (t=_task_list; t!=NULL; t=t->next) {
        kprintf (" %04d %04d %-17s     %c   %4d  %08X  %08X\n", t->pid, t->ppid, t->name, t->state, t->priority, t->ringticks[0], t->ringticks[3]);
      }
      kprintf ("\n");

    }
  }


  pid = sys_fork ();
  if (pid == 0) {
    kprintf ("PID[%05d] After second fork() : Child says %d\n", _current_task->pid, pid);

    strncpy (_current_task->name, "Test process 2\0", 15);

    while (1) {
      kprintf ("Y");
      sys_sleep (300);
      kprintf ("y");
      sys_sleep (300);
    }
  }


  // This is the idle-process from now on.
  kprintf ("user_idle() on main pid (%d)\n", _current_task->pid);
  user_idle ();
}


/************************************
 * Switch to construct and deadlock the system
 */
void kdeadlock (void) {
  switch_console (_kconsole, 1);
  kprintf ("\nkdeadlock(): Deadlocked the system.\n");

  // Stop interrupts and halt processor. That should deadlock the system decently enough
  cli ();
  hlt ();

  // We should not be here. But in case we do, just "hang"
  while (1) ;
}

/************************************
 * Only print when we can print
 */
int kprintf_help (char c, void **ptr) {

#ifdef __DEBUG__
  // Bochs debug output
  outb (0xE9, c);
#endif

  // Kernel messages go direct the construct
  con_putch (_kconsole, c); // Write to the construct

  // Also write to the current screen if it's not already on the main screen
  if (_current_task && _current_task->console_ptr->index != CON_KERNEL_IDX) con_putch (_current_task->console_ptr, c);
  return 0;
}

/************************************
 * Prints on the construct console (but we don't switch to it)
 */
void kprintf (const char *fmt, ...) {
  va_list args;

  va_start (args, fmt);
  (void)do_printf (fmt, args, kprintf_help, NULL);
  va_end (args);

  con_flush (_kconsole);
}

/************************************
 * Panics the system.
 */
void kpanic (const char *fmt, ...) {
  va_list args;

  cli();

  // Make sure we are currently seeing the construct
  switch_console (_kconsole, 1);

  kprintf ("\n[KRN] KERNEL PANIC: ");
  va_start (args, fmt);
  do_printf (fmt, args, kprintf_help, NULL);
  va_end (args);

  con_flush (_kconsole);
  kdeadlock();
}


/************************************
 * Prints on the construct console (but we don't switch to it)
 */
int tprintf_help (char c, void **ptr) {
  // Kernel messages go direct the construct
  __asm__ __volatile__ ("int	$" SYSCALL_INT_STR " \n\t" : : "a" (SYS_CONWRITE), "b" (c), "c" (0) );
  return 0;
}

void tprintf (const char *fmt, ...) {
  va_list args;

  va_start (args, fmt);
  (void)do_printf (fmt, args, tprintf_help, NULL);
  va_end (args);

  __asm__ __volatile__ ("int	$" SYSCALL_INT_STR " \n\t" : : "a" (SYS_CONFLUSH));
}

/************************************
 * The kernel constructor. Task 1 (the primary task after the kernel idle
 * task) loads the constructor which operates on console 1.
 */
void construct (void) {
  char s[256];

  kprintf ("Welcome to the construct. deadlocking\n");

  // Remove me...
  while (1) ;


  // Repeat until heel lang
  while (1) {
    con_printf (_kconsole, "\nKRN> ");
    con_gets (_kconsole, s, 255);
    con_printf (_kconsole, "\n");

    if (strcmp (s, "help")==0) {
        con_printf (_kconsole, " Commands: \n");
        con_printf (_kconsole, "\n");
        con_printf (_kconsole, " [ ] reboot                Reboots system.\n");
        con_printf (_kconsole, " [ ] pit [frequency]       Set IRQ0 (timer) interval to freqency or shows\n");
        con_printf (_kconsole, "                           current frequency.\n");
        con_printf (_kconsole, " [ ] echo [tty] [string]   Prints 'string' to console number 'tty'\n");
        con_printf (_kconsole, " [*] tasks                 Shows task list\n");
        con_printf (_kconsole, " [*] gdt                   Shows Global Descriptor Table information\n");
        con_printf (_kconsole, " [ ] idt                   Shows Interrupt Descriptor Table information\n");
        con_printf (_kconsole, "\n");
        con_printf (_kconsole, " [*] Functioning    [ ] Not implemented\n");
    } else
    if (strcmp (s, "reboot")==0) {
      con_printf (_kconsole, "REBOOT not implemented.\n");

    } else if (strcmp (s, "task")==0) {
      con_printf (_kconsole, "PID     TASK                STAT  PRIO  CON    RT0  RT1  RT2  RT3\n");
      con_printf (_kconsole, "-----------------------------------------------------------------\n");
      CYBOS_TASK *t;
      for (t=_task_list; t!=NULL; t=t->next) {
        con_printf (_kconsole, "%5d   %-17s    %1d  %4d   %4d   %04X %04X %04X %04X\n", t->pid, t->name, t->state, t->priority, t->console_ptr->index, t->ringticks[0],t->ringticks[1],t->ringticks[2],t->ringticks[3]);
      }

    } else if (strcmp (s, "echo")==0) {
      con_printf (_kconsole, "ECHO not implemented.\n");
    } else if (strcmp (s, "pit")==0) {
      con_printf (_kconsole, "PIT not implemented.\n");
    } else if (strcmp (s, "gdt")==0) {
      cmd_print_gdt (0);
    } else if (strcmp (s, "idt")==0) {
      con_printf (_kconsole, "IDT not implemented.\n");
    } else {
      con_printf (_kconsole, "Command not found. Type 'help' for commands.\n");
    }
  }
}

/*******************************************************
 * Prints a numeric counter in an endless loop
 */
void task_2 (void) {
  int t2c = 0;

  while (1) {
    tprintf ("I: %d\n", t2c++);
  }
}

/*******************************************************
 * Prints 'A-Z' in an endless loop
 */
void task_1 (void) {
  char t1c = 'A';

  while (1) {
    if (++t1c >= 'Z') t1c = 'A';
    tprintf ("%c", t1c);
  }
}

