/******************************************************************************
 *
 *  File        : idt.c
 *  Description : Interrupt routines and managment.
 *
 *****************************************************************************/
#include "kernel.h"
#include "general.h"
#include "errors.h"
#include "conio.h"
#include "console.h"
#include "kmem.h"
#include "paging.h"
#include "schedule.h"
#include "tss.h"
#include "gdt.h"
#include "idt.h"
#include "io.h"


CYBOS_TASK idle_task;                // Idle task to which the initial
                                     // taskswitch places the single-taking
                                     // TSS structure in.

CYBOS_TASK *_current_task = NULL;    // Current active task on the CPU.
CYBOS_TASK *_task_list = NULL;       // Points to the first task in the tasklist (idle task)


TSS tss_entry;
//LDT ldt_entry;

//unsigned char charstates[] = "IrRSUZ";

int current_pid = PID_IDLE - 1;      // First call to allocate_pid will return PID_IDLE

void task_switch (int tss_selector); // Found in task.S
void tprintf (const char *fmt, ...);


/*******************************************************
 * Initialises the multitasking environment by creating
 * the kernel task. After the multitasking, the function
 * loads this task by setting it's eip to the point right
 * after loading the task. This way we 'jump' to the task,
 * about the same way we 'jump' into protected mode.
 *
 */
int sched_init () {
  // Clear TSS
  memset (&tss_entry, 0, sizeof (TSS));

  // Set general TTS in the GDT. This is used for task switching (sort of)
  Uint64 tss_descriptor = gdt_create_descriptor ((int)&tss_entry, (int)sizeof (TSS), GDT_PRESENT+GDT_DPL_RING0+GDT_NOT_BUSY+GDT_TYPE_TSS, GDT_NO_GRANULARITY+GDT_AVAILABLE);
  gdt_set_descriptor (TSS_TASK_DESCR, tss_descriptor);

  tss_entry.ss0  = SEL(KERNEL_DATA_DESCR, TI_GDT+RPL_RING0);
  tss_entry.esp0 = 0x00;

  tss_entry.cs = SEL(KERNEL_CODE_DESCR, TI_GDT+RPL_RING3);
  tss_entry.ss = SEL(KERNEL_DATA_DESCR, TI_GDT+RPL_RING3);

    // Here we set the cs, ss, ds, es, fs and gs entries in the TSS. These specify what
    // segments should be loaded when the processor switches to kernel mode. Therefore
    // they are just our normal kernel code/data segments - 0x08 and 0x10 respectively,
    // but with the last two bits set, making 0x0b and 0x13. The setting of these bits
    // sets the RPL (requested privilege level) to 3, meaning that this TSS can be used
    // to switch to kernel mode from ring 3.
    tss_entry.cs   = 0x0b;
    tss_entry.ss = tss_entry.ds = tss_entry.es = tss_entry.fs = tss_entry.gs = 0x13;

  // Load/flush task register
  __asm__ __volatile__ ( "ltrw %%ax\n\t" : : "a" (TSS_TASK_SEL));


  /* We initialize the idle task and make it the base of our task_list. Basically,
   * the thing we are running now is the idle_task. */

  /* The idle task is the first task. We need to set this because create_user_task()
   * calls allocate_pid, which scans through the _task_list. We need to make sure it's
   * set to "something" (or in this case, the (uninitialized) idle-task itself, which
   * would be the only task. This will make allocate_pid behave correctly). */
  _task_list = &idle_task;
  memset (&idle_task, 0, sizeof (CYBOS_TASK));

  // Note that the idle-task has an illegal EIP. On the first taskswitch, the data in this
  // idle-task tss will be overwritten by the current CPU info so the eip will be correct
  // again. This is the only task that has an illegal EIP.
  sched_create_kernel_task (&idle_task, 0x0, "Idle task", NO_CREATE_CONSOLE);
  idle_task.priority = PRIO_LOWEST;         // It has the lowest priority

  // The idle task gets the main console
  idle_task.console = _kconsole;

  idle_task.ebp = 0;
  idle_task.esp = 0;
  idle_task.eip = 0;
  idle_task.page_directory = _current_pagedirectory;

  // Load task register
  // __asm__ __volatile__ ( "lldt %%ax\n\t" : : "a" (LDT_TASK_SEL));

  return ERR_OK;
}





/*******************************************************
 * Adds a task to the linked list of tasks.
 */
void sched_add_task (CYBOS_TASK *new_task) {
  CYBOS_TASK *last_task = _task_list;

  // No tasks found, this task will be the first. This is only true when adding the primary task (idle-task).
  if (last_task == NULL) {
    _task_list = new_task;  // Let task_list point to the first task
    new_task->next = NULL;  // No next tasks in the list
    new_task->prev = NULL;  // And also no previous tasks in the list

  } else {
    // Find last task in the list
    while (last_task->next != NULL) last_task = (CYBOS_TASK *)last_task->next;

    // Add to the list
    last_task->next = new_task;       // End of link points to the new task
    if (last_task != new_task) {
      new_task->prev = last_task;     // Create a link back if it's not the first task
    }
    new_task->next = NULL;            // And the new task points to the end of line..
  }
}

/*******************************************************
 * Removes a task from the linked list of tasks.
 */
void sched_remove_task (CYBOS_TASK *task) {
  CYBOS_TASK *tmp,*tmp1;

  // Don't remove idle_task!
  if (task->pid == PID_IDLE) kpanic ("Cannot delete idle() task!");

  // Simple remove when it's the last one on the list.
  if (task->next == NULL) {
    tmp = (CYBOS_TASK *)task->prev;
    tmp->next = NULL;

  // Simple remove when it's the first one on the list (this should not be possible since it would be the idle-task)
  } else if (task->prev == NULL) {
    _task_list = task->next;  // Next first task is the start of the list
    task->prev = NULL;

  // Otherwise, remove the task out of the list
  } else {
    tmp = task->prev;        // tmp = item1
    tmp->next = task->next;  // Link item1->next to item3
    tmp1 = task->next;       // tmp1 = item3
    tmp1->prev = tmp;        // Link item3->prev to item1
  }

}




/*******************************************************
 * Initialises a CYBOS_TASK structure with correct values
 */
int _create_task (CYBOS_TASK *task, int kernel_or_usertask,
                  Uint32 cs_selector, Uint32 ds_selector,
                  Uint32 ss0, Uint32 esp0,
                  Uint32 ss3, Uint32 esp3,
                  Uint32 eip, TPAGEDIRECTORY *page_directory,
                  char *taskname, int console) {

  // We are initialising at the moment
  task->state = TASK_STATE_INITIALISING;

  // Set backlink and stack values
//  task->tss.link = 0;

//  __asm__ __volatile__ ("mov %%esp, %0" : "=r" (esp0));
//  __asm__ __volatile__ ("mov %%esp, %0" : "=r" (esp3));

//  task->tss.esp0 = esp0;      // Ring 0 stack (kernelmode)
//  task->tss.ss0  = ss0;
//  task->tss.esp1 = esp3;      // Ring 1 stack (not used)
//  task->tss.ss1  = ss3;
//  task->tss.esp2 = esp3;      // Ring 2 stack (not used)
//  task->tss.ss2  = ss3;
//  task->tss.esp  = esp3;      // Ring 3 stack (usermode)
//  task->tss.ss   = ss3;

//  // Page directory
//  task->tss.cr3 = (Uint32)page_directory->physical_address;
//  task->esp =
//  task->ss =
  //  task->cr3 = (Uint32)page_directory->physical_address;
  task->page_directory = page_directory;

//  // Program counter and flags
//  task->tss.eip    = eip;
//  task->tss.eflags = 0x4202;     // Must be at least 0x0002
//
//  // Initialize the global vars to default values
//  task->tss.eax = 0;
//  task->tss.ebx = 0;
//  task->tss.ecx = 0;
//  task->tss.edx = 0;
//  task->tss.ebp = 0;
//  task->tss.esi = 0;
//  task->tss.edi = 0;
//
//  // Set selectors
//  task->tss.cs = cs_selector;
//  task->tss.ds = ds_selector;
//  task->tss.es = ds_selector;
//  task->tss.fs = ds_selector;
//  task->tss.gs = ds_selector;
//
//
//  // Various TSS stuff
////  task->tss.ldtr = LDT_TASK_SEL;
//  task->tss.iopb_offset = 0;
//  task->tss.T = 0;


//  // Create TSS descriptor. Don't save it into the GDT, we store it manuallly in our task
//  task->tss_entry = gdt_create_descriptor ((int)&task->tss, (int)sizeof (TSS), GDT_PRESENT+GDT_DPL_RING0+GDT_NOT_BUSY+GDT_TYPE_TSS, GDT_NO_GRANULARITY+GDT_AVAILABLE);

//  // Create LDT descriptor and selector. Also store in this task
//  task->ldt_entry = gdt_create_descriptor ((int)&task->ldt, (int)8 * USERTASK_LDT_SIZE, GDT_PRESENT+GDT_DPL_RING0+GDT_NOT_BUSY+GDT_TYPE_LDT, GDT_NO_GRANULARITY+GDT_AVAILABLE);


/*
  // Create an LDT
  if (kernel_or_usertask == TASK_USER) {
    // TODO: Use gdt_create_descriptor for this as well..
    //task->ldt[0] = gdt_create_descriptor (0x0, 0xFFFFF, GDT_PRESENT+GDT_DPL_RING3+GDT_NOT_BUSY+GDT_TYPE_CODE, GDT_GRANULARITY+GDT_AVAILABLE);
    //task->ldt[1] = gdt_create_descriptor (0x0, 0xFFFFF, GDT_PRESENT+GDT_DPL_RING3+GDT_NOT_BUSY+GDT_TYPE_DATA, GDT_GRANULARITY+GDT_AVAILABLE);
    task->ldt[USER_CODE_DESCR] = 0x00cffa000000ffffULL;  // Full 4GW code segment (ring3)
    task->ldt[USER_DATA_DESCR] = 0x00cff2000000ffffULL;  // Full 4GW data segment (ring3)
  } else {
    // TODO: Use gdt_create_descriptor for this as well..
    //task->ldt[0] = gdt_create_descriptor (0x0, 0xFFFFF, GDT_PRESENT+GDT_DPL_RING0+GDT_NOT_BUSY+GDT_TYPE_CODE, GDT_GRANULARITY+GDT_AVAILABLE);
    //task->ldt[1] = gdt_create_descriptor (0x0, 0xFFFFF, GDT_PRESENT+GDT_DPL_RING0+GDT_NOT_BUSY+GDT_TYPE_DATA, GDT_GRANULARITY+GDT_AVAILABLE);
    task->ldt[USER_CODE_DESCR] = 0x00cf8a000000ffffULL;  // Full 4GW code segment (ring0)
    task->ldt[USER_DATA_DESCR] = 0x00cf82000000ffffULL;  // Full 4GW data segment (ring0)
  }
*/

  // @TODO: No permission to use IO on ring 1 or above
//  for (i=0; i!=8192; i++) task->iomap[i]=0;

  // Default priority
  task->priority = PRIO_DEFAULT;
  task->pid = allocate_new_pid ();
  task->ppid = PID_IDLE;

  // Name of the task
  strncpy (task->name, taskname, 49);

  // Times spend in each CPL ring (only 0 and 3 are used)
  task->ringticksHi[0] = task->ringticksLo[0] = 0;
  task->ringticksHi[1] = task->ringticksLo[1] = 0;
  task->ringticksHi[2] = task->ringticksLo[2] = 0;
  task->ringticksHi[3] = task->ringticksLo[3] = 0;

  // Set console pointer
  if (console == CREATE_CONSOLE) {
    task->console = create_console (task->name, 1);
  } else {
    task->console = NULL;
  }

  // Add task to schedule-switcher. We are still initialising so it does not run yet.
  sched_add_task (task);

  // We are all done. Available for scheduling
  task->state = TASK_STATE_RUNNABLE;

  // All done
  return ERR_OK;
}

// ==========================================================================================
int sched_create_kernel_task (CYBOS_TASK *task, Uint32 eip, char *taskname, int console) {

  // Create kernel and user stack (TODO: need a kernel stack to create? no.. i do not think so?)
  kprintf ("Sched_create_kernel_task()  create stacks\n");
  task->kstack = (Uint32)kmalloc_pageboundary (KERNEL_STACK_SIZE);
  task->ustack = (Uint32)kmalloc_pageboundary (USER_STACK_SIZE);

  // Stacks are made. userstack can be completely empty but the kernel stack should be copied from
  // the current stack and the relative vars should be changed (why?)

  return _create_task (task, TASK_KERNEL,
                       SEL(KERNEL_CODE_DESCR, TI_GDT+RPL_RING0),
                       SEL(KERNEL_DATA_DESCR, TI_GDT+RPL_RING0),
                       SEL(KERNEL_DATA_DESCR, TI_GDT+RPL_RING0), (task->kstack + KERNEL_STACK_SIZE-1),
                       SEL(KERNEL_DATA_DESCR, TI_GDT+RPL_RING0), (task->ustack + USER_STACK_SIZE-1),
                       eip, _current_pagedirectory,
                       taskname, console);
}

/*
// ==========================================================================================
int sched_create_user_task (CYBOS_TASK *task, Uint32 eip, char *taskname, int console) {
  // Create kernel and user stack
  task->kstack = (Uint32)kmalloc_pageboundary (KERNEL_STACK_SIZE);
  task->ustack = (Uint32)kmalloc_pageboundary (USER_STACK_SIZE);

  task->ustack = 0x40050000;

  // Stacks made. userstack can be completely empty but the kernel stack should be copied from
  // the current stack and the relative vars should be changed (why?)

  return _create_task (task, TASK_USER,
                       SEL(  USER_CODE_DESCR, TI_LDT+RPL_RING3),
                       SEL(  USER_DATA_DESCR, TI_LDT+RPL_RING3),
                       SEL(KERNEL_DATA_DESCR, TI_GDT+RPL_RING0), (task->kstack + KERNEL_STACK_SIZE),
                       SEL(  USER_DATA_DESCR, TI_LDT+RPL_RING3), (task->ustack + USER_STACK_SIZE),
                       eip, _current_pagedirectory,
                       taskname, console);
}
*/


/*******************************************************
 * Fetch the next task in the list, or restart from the beginning
 */
CYBOS_TASK *get_next_task (CYBOS_TASK *cur_task) {
  if (cur_task->next == NULL) {
    return (CYBOS_TASK *)_task_list;       // Return first task is no next tasks are found
  } else {
    return (CYBOS_TASK *)cur_task->next;
  }
}





// ========================================================================================
// bit scan forward
int bsf (int bit_field) {
  int bit_index;

  // If bit_field is empty, unpredictable results is returned. Always return with -1.
  if (bit_field == 0) return -1;

  // Could be done in C, but we do it in asm..
  __asm__ __volatile__ ("bsfl %%eax, %%ebx" : "=b" (bit_index) : "a" (bit_field));
  return bit_index;
}

// Bit test & reset
int btr (int bit_field, int bit_index) {
  int new_bit_field;

  // Could be done in C, but we do it in asm..
  __asm__ __volatile__ ("btr %%ebx, %%eax" : "=a" (new_bit_field) : "a" (bit_field), "b" (bit_index));
  return new_bit_field;
}

// Bit test & set
int bts (int bit_field, int bit_index) {
  int new_bit_field;

  // Could be done in C, but we do it in asm..
  __asm__ __volatile__ ("bts %%ebx, %%eax" : "=a" (new_bit_field) : "a" (bit_field), "b" (bit_index));
  return new_bit_field;
}


// ========================================================================================
void sys_signal (CYBOS_TASK *task, int signal) {
//  kprintf ("\nsys_signal (PID %d   SIG %d)\n", task->pid, signal);
  // task->signal |= (1 << signal);
  task->signal = bts (task->signal, signal);
}



// ======================================================
// Checks for signals in the current task, and act accordingly
void handle_pending_signals (void) {
//  kprintf ("sign handling (%d)\n", _current_task->pid);
  if (_current_task == NULL) return;

  // No pending signals for this task. Do nothing
  if (_current_task->signal == 0) return;

  /*
    Since we scan signals forward, it means the least significant bit has the highest
    priority. So the lower the signal number (ie: the bit in the field), the higher
    it's priority will be.
  */

  int signal = bsf (_current_task->signal);                       // Fetch signal
  _current_task->signal = btr (_current_task->signal, signal);    // Clear this signal from the list. Next time we can do another signal.


// TODO: handle like this:  _current_task->sighandler[signal]
  switch (signal) {
    case SIGHUP :
                   kprintf ("SIGHUP");
                   break;

    case SIGALRM :
//                   kprintf ("SIGALRM");
                   break;
    default :
                   kprintf ("Default handler");
                   break;

  }
}


// ======================================================
// Handles stuff for all tasks in each run. Checking alarms,
// waking up on signals etc.
void global_task_administration (void) {
  CYBOS_TASK *task;

  // Do all tasks
  for (task = _task_list; task != NULL; task = task->next) {
    // This task is not yet ready. Don't do anything with it
    if (task->state == TASK_STATE_INITIALISING) continue;

//    kprintf ("PID %d  ALRM: %d  SIG: %d  STATE: %c\n", task->pid, task->alarm, task->signal, task->state);

/*
    // Do priorities of the tasks
    if (task->state == TASK_STATE_RUNNING) {
      if ( (task->priority += 30) <= 0) {
        task->priority = -1;
      } else {
        task->priority -= 10;
      }
    }
*/

    // Check for alarm. Decrease alarm and send a signal if time is up
    if (task->alarm) {
      task->alarm--;
      if (task->alarm == 0) {
        kprintf ("Alarma on pid %d !!!\n", task->pid);
        sys_signal (task, SIGALRM);
      }
    }

    // A signal is found and the task can be interrupted. Set the task to be ready again
    if (task->signal && task->state == TASK_STATE_INTERRUPTABLE) {
      task->state = TASK_STATE_RUNNABLE;
    }
  }
}




/*******************************************************
 *
 */
void switch_task (void) {
  CYBOS_TASK *previous_task;
  CYBOS_TASK *next_task;
  Uint32 esp, ebp, eip, cr3;

  // There is a signal pending. Do that first before we actually run code again (TODO: is this really a good idea?)
  if (_current_task->signal != 0) return;

  // This is the task we're running. It will be the old task after this
  previous_task = next_task = _current_task;

  /* Don't use _current_task from this point on. We define previous_task as the one we are
   * about to leave, and next_task as the one we are about to enter. */

  // Read esp, ebp now for saving later on.
  asm volatile("mov %%esp, %0" : "=r"(esp));
  asm volatile("mov %%ebp, %0" : "=r"(ebp));

//  kprintf ("ESP: %08X\n", esp);
//  kprintf ("EBP: %08X\n", ebp);

  // Read the current instruction address.
  eip = read_eip();

  // We are now called in 2 different states.
  //  1. as the next task (eip = 0xDEADBEEF), so we're done
  //  2. as the previous task just wanting to fetch eip (!= 0xDEADBEEF)
  if (eip == 0xDEADBEEF) return;

//  kprintf ("Child switch deadbeef\n");

  // Save registers. The rest is saved by the interrupt call and restored by IRET
  previous_task->eip = eip;
  previous_task->esp = esp;
  previous_task->ebp = ebp;


  // Fetch the next available task
  do {
    next_task = get_next_task (next_task);
  } while (next_task->state != TASK_STATE_RUNNABLE);    // Hmmz.. When no runnable tasks are found,
                                                        // we should automatically fetch idletask()?

  // Looks like we do not need to switch (maybe only 1 task, or still idle?
  if (previous_task == next_task) {
//    kprintf ("Returning same task (PID %d)\n", next_task->pid);
    return;
  }

  // Only set this task to be available again if it was still running. If it's
  // sleeping TASK_STATE_(UN)INTERRUPTIBLE), then don't change this setting.
  if (previous_task->state == TASK_STATE_RUNNING) {
    previous_task->state = TASK_STATE_RUNNABLE;
  }

  // Old task is available again. New task is running
  next_task->state = TASK_STATE_RUNNING;

  // Fetch the new registers
  eip = next_task->eip;
  esp = next_task->esp;
  ebp = next_task->ebp;
  cr3 = next_task->page_directory->physical_address;

  // Set the kernel stack
//  kprintf ("SWITCH(%d) Stack on %08X (%08X) (ESP: %08X)\n", next_task->pid, next_task->kstack, next_task->kstack + KERNEL_STACK_SIZE, esp);
  tss_set_kernel_stack (next_task->kstack + KERNEL_STACK_SIZE);

//  kprintf ("Switching task to pid %d\n", next_task->pid);
//  kprintf ("EIP: %08X\n", eip);
//  kprintf ("ESP: %08X\n", esp);
//  kprintf ("EBP: %08X\n", ebp);
//  kprintf ("CR3: %08X\n", cr3);


  // The next task is now the current task..
  _current_task = next_task;

  // Switch to a new task (start right after read_eip()).
  __asm__ __volatile__ ("cli;                    \
                         mov %1, %%esp;       \
                         mov %2, %%ebp;       \
                         mov %3, %%cr3;       \
                         mov $0xDEADBEEF, %%eax; \
                         sti;                    \
                         jmp *%%edx  "
                         : : "d"(eip), "r"(esp), "r"(ebp), "r"(cr3));
}


// ===============================================================
// Switches to usermode. On return we are no longer in the kernel
// but in the RING3 usermode. Make sure we have done all kernel
// setup before this.
//
void switch_to_usermode (void) {
  // current_task points to idle_task. This also allows the scheduler (sched_switch) to
  // see that we are up and running. As soon as _current_task != NULL, it can switch
  // to other tasks (if there are any of course)
  _current_task = _task_list;

  // Set the kernel stack
  tss_set_kernel_stack(_current_task->kstack + KERNEL_STACK_SIZE);

  // Move from kernel ring0 to usermode ring3 AND start interrupts at the same time.
  // This is needed because we cannot use sti() inside usermode. Since there is no 'real'
  // way of reaching usermode, we have to 'jumpstart' to it by creating a stackframe which
  // we return from. When the IRET we return to the same spot, but in a different ring.
  asm volatile("  movw   %%cx, %%ds; \n\t" \
               "  movw   %%cx, %%es; \n\t" \
               "  movw   %%cx, %%fs; \n\t" \
               "  movw   %%cx, %%gs; \n\t" \

               "  movl   %%esp, %%eax\n\t" \
               "  pushl  %%ecx; \n\t" \
               "  pushl  %%eax; \n\t" \
               "  pushfl; \n\t" \
               "  popl   %%eax\n\t" \
               "  or     $0x200, %%eax\n\t" \
               "  pushl  %%eax\n\t" \
               "  pushl  %%ebx; \n\t" \
               "  pushl  $1f; \n\t" \
               "  iret; \n\t" \
               "1: \n\t" \
  			       ::"b"(SEL(KUSER_CODE_DESCR,TI_GDT+RPL_RING3)),
			           "c"(SEL(KUSER_DATA_DESCR,TI_GDT+RPL_RING3)):"eax");
}


// ========================================================================================
int allocate_new_pid (void) {
  CYBOS_TASK *tmp;
  int b;

  do {
    b = 0;    // By default, we assume current_pid points to a free PID

    // Increase current PID or wrap around
    current_pid = (current_pid < MAX_PID) ? current_pid+1 : PID_IDLE;

    // Check if there is a process currently running with this PID
    for (tmp = _task_list; tmp->next != NULL && b != 1; tmp = tmp->next) {
      if (tmp->pid == current_pid) b = 1;
    }
  } while (b == 1);

  return current_pid;
}


// ========================================================================================
int sys_sleep (int ms) {
  if (_current_task == &idle_task) kpanic ("Cannot sleep idle task!");

  _current_task->alarm = ms;
  _current_task->state = TASK_STATE_INTERRUPTABLE;

  kprintf ("Sleeping process %d\n", _current_task->pid);

  // We're sleeping. So go to a next task.
  scheduler ();

  return 0;
}


// ========================================================================================
int getppid (void) {
  int ret;
  __asm__ __volatile__ ("int	$" SYSCALL_INT_STR " \n\t" : "=a" (ret) : "a" (SYS_GETPPID) );
  return ret;
}

// ========================================================================================
int getpid (void) {
  int ret;
  __asm__ __volatile__ ("int	$" SYSCALL_INT_STR " \n\t" : "=a" (ret) : "a" (SYS_GETPID) );
  return ret;
}

// ========================================================================================
int fork (void) {
  int ret;
  __asm__ __volatile__ ("int	$" SYSCALL_INT_STR " \n\t" : "=a" (ret) : "a" (SYS_FORK) );
  return ret;
}

// ========================================================================================
int sleep (int ms) {
  int ret;
  __asm__ __volatile__ ("int	$" SYSCALL_INT_STR " \n\t" : "=a" (ret) : "a" (SYS_SLEEP) );
  return ret;
}

// ========================================================================================
int idle () {
  int ret;
  __asm__ __volatile__ ("int	$" SYSCALL_INT_STR " \n\t" : "=a" (ret) : "a" (SYS_IDLE) );
  return ret;
}

// ========================================================================================
int sys_idle () {
  /* We can only idle when we are the 0 (idle) task. Other tasks cannot idle, because if
   * they are doing nothing (for instance, they are waiting on a signal (sleep, IO etc), they
   * should not idle themself, but other processes will take over (this is taken care of by
   * the scheduler). So anyway, when there is absolutely no process available that can be
   * run (everybody is waiting), the scheduler will choose the idle task, which only job is
   * to call this function (over and over again). It will put the processor into a standby
   * mode and waits until a IRQ arrives. It's way better to call a HLT() than to do a
   * "for(;;) ;".. */
  if (_current_task->pid != 0) {
    kprintf ("Warning.. only PID 0 can idle...\n");
    return -1;
  }

  kprintf ("HLT()ing system until IRQ\n");
  sti();
  hlt();
  return 0;
}

// ========================================================================================
int sys_getpid (void) {
  return _current_task->pid;
}

// ========================================================================================
int sys_getppid (void) {
  return _current_task->ppid;
}


// ========================================================================================
int sys_fork (void) {
  CYBOS_TASK *child_task, *parent_task;

  cli ();

  // The current task will be the parent task
  parent_task = _current_task;

  // Create a new task
  child_task = (CYBOS_TASK *)kmalloc (sizeof (CYBOS_TASK));

  // copy all data from the parent into the child
  memcpy (child_task, parent_task, sizeof (CYBOS_TASK));

  // The page directory is the cloned space
  child_task->page_directory = clone_pagedirectory (parent_task->page_directory);


  // Available for scheduling
  child_task->state = TASK_STATE_INITIALISING;

  child_task->pid  = allocate_new_pid ();           // Make new PID
  child_task->ppid = parent_task->pid;              // Set the parent pid

  // Reset task times for the child
  child_task->ringticksHi[0] = child_task->ringticksLo[0] = 0;
  child_task->ringticksHi[1] = child_task->ringticksLo[1] = 0;
  child_task->ringticksHi[2] = child_task->ringticksLo[2] = 0;
  child_task->ringticksHi[3] = child_task->ringticksLo[3] = 0;

  child_task->esp = 0;
  child_task->ebp = 0;
  child_task->eip = 0;

kprintf ("sys_fork() Creating kernel stack for this process\n");
  Uint32 addr;
  child_task->kstack = (Uint32)kmalloc_pageboundary_physical (KERNEL_STACK_SIZE, &addr);
  kprintf ("KSTACK IS : %08X   Phys: %08X\n",child_task->kstack, addr);

//  clone_debug = 1;
//  clone_pagedirectory (parent_task->page_directory);

  // Add task to schedule-switcher. We are still initialising so it does not run yet.
  sched_add_task (child_task);

  // We set the child task to start right after read_eip.
  Uint32 eip = read_eip();


  // From this point on, 2 tasks will be executing this code. The parent will come first and
  // will set the child_task stuff we need. After that it will set the child_task state to
  // TASK_STATE_RUNNABLE. From that point only we could run this code again. Since the
  // current task is at that point NOT the same as the parent_task, we can actually distinguish
  // between parent en child.
  if (_current_task == parent_task) {
    Uint32 esp; asm volatile("mov %%esp, %0" : "=r"(esp));
    Uint32 ebp; asm volatile("mov %%ebp, %0" : "=r"(ebp));
    child_task->esp = esp;    // The stacks for the child are the same (virtual) address.
    child_task->ebp = ebp;    // The clone page directory already took care of copying userdata.
    child_task->eip = eip;    // This is the place we we left of (just right after read_eip() a few lines above)

    // Available for scheduling
    child_task->state = TASK_STATE_RUNNABLE;

    // Ok to interrupt us again
    sti ();

    return child_task->pid;   // Return the PID of the child
  } else {
    sti ();
    return 0;     // This is the child. Return 0
  }
}


// ================================================================================
void scheduler (void) {
  global_task_administration ();       /* Sort priorities, alarms, wake-up-on-signals etc */
  switch_task ();                      /* Switch to another task */
  handle_pending_signals ();           /* Handle pending signals for current task */
}

