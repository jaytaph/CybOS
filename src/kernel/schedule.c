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
#include "queue.h"


CYBOS_TASK *_current_task = NULL;    // Current active task on the CPU.
QUEUE _task_list;                    // Points to the first task in the tasklist (which should be the idle task)
QUEUE _running_list;

TSS tss_entry;                       // Memory to hold the TSS. Address is loaded into the GDT

int current_pid = PID_IDLE - 1;      // First call to allocate_pid will return PID_IDLE




// void task_switch (int tss_selector); // Found in task.S
void tprintf (const char *fmt, ...);

// @TODO: Make this inline ??
void context_switch (Uint32 *old_esp, Uint32 new_esp);    // Found in page.S


/**
 * Creates a new thread (process)
 */
void thread_create_kernel_thread (Uint32 start_address, char *taskname, int console) {
  // Create room for task
  CYBOS_TASK *task = (CYBOS_TASK *)kmalloc (sizeof (CYBOS_TASK));

  // Create kernel stack and setup "start" stack that gets pop'ed by context_switch()
  task->kstack = (Uint32)kmalloc_pageboundary (KERNEL_STACK_SIZE);
  task->esp = task->kstack + KERNEL_STACK_SIZE;

  Uint32 *stacktop = (Uint32 *)task->esp;

  stacktop--;
  *stacktop = start_address;
  int i;
  for (i=0; i<8; i++) {
    stacktop--;
    *stacktop = 0;
  }
  task->esp = (Uint32)stacktop;

  // Set page directory
  task->page_directory = _current_pagedirectory;

  // PID's + priority
  task->priority = PRIO_DEFAULT;
  task->pid = allocate_new_pid ();
  task->ppid = 0;

  // Name of the task
  strncpy (task->name, taskname, 49);

  // Times spend in each CPL ring (only 0 and 3 are used)
  task->ringticksHi[0] = task->ringticksLo[0] = 0;
  task->ringticksHi[1] = task->ringticksLo[1] = 0;
  task->ringticksHi[2] = task->ringticksLo[2] = 0;
  task->ringticksHi[3] = task->ringticksLo[3] = 0;

  // Set console pointer to kernel console
  // Set console pointer
  if (console == CONSOLE_CREATE_NEW ) {
    task->console = create_console (task->name, 1);
  } else if (console == CONSOLE_USE_KCONSOLE) {
    task->console = _kconsole;
  } else {
    task->console = NULL;
  }

  // We are initialising this task at the moment
  task->state = TASK_STATE_INITIALISING;

  // Add task to schedule-switcher. We are still initialising so it does not run yet.
  sched_add_task (task);

  // Add to running queue
  sched_add_runnable_task (task);

  // We are all done. Available for scheduling
}



/*******************************************************
 * Adds a task to the linked list of tasks.
 */
void sched_add_task (CYBOS_TASK *new_task) {
  int state = disable_ints ();
  queue_add_item (&_task_list, new_task);
  restore_ints (state);
}

/*******************************************************
 * Removes a task from the linked list of tasks.
 */
void sched_remove_task (CYBOS_TASK *task) {
  // Don't remove idle_task!
  if (task->pid == PID_IDLE) kpanic ("Cannot delete idle task!");

  int state = disable_ints ();
  queue_del_item (&_task_list, task);
  restore_ints (state);
}

/*******************************************************
 *
 */
void sched_add_runnable_task (CYBOS_TASK *task) {
  task->state = TASK_STATE_RUNNABLE;

  int state = disable_ints ();
  queue_add_item (&_running_list, task);
  restore_ints (state);
}

/*******************************************************
 *
 */
void sched_remove_runnable_task (CYBOS_TASK *task) {
  int state = disable_ints ();
  queue_del_item (&_running_list, task);
  restore_ints (state);
}

/*******************************************************
 * Fetch the next task in the list, or restart from the beginning
 */
CYBOS_TASK *get_next_runnable_task () {
  CYBOS_TASK *next_task;

  int state = disable_ints ();

  queue_reset (&_running_list);
  next_task = queue_next (&_running_list);

  restore_ints (state);

  return next_task;
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


/****
 *
 */
void sys_signal (CYBOS_TASK *task, int signal) {
//  kprintf ("\nsys_signal (PID %d   SIG %d)\n", task->pid, signal);
  // task->signal |= (1 << signal);
  task->signal = bts (task->signal, signal);
}

/****
 * Checks for signals in the current task, and act accordingly
 */
void handle_pending_signals (void) {
  int state = disable_ints ();

//  kprintf ("sign handling (%d)\n", _current_task->pid);
  if (_current_task == NULL) {
    restore_ints (state);
    return;
  }

  // No pending signals for this task. Do nothing
  if (_current_task->signal == 0) {
    restore_ints (state);
    return;
  }

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

  restore_ints (state);
}


/****
 * Handles stuff for all tasks in each run. Checking alarms,
 * waking up on signals etc.
 */
void global_task_administration (void) {
  CYBOS_TASK *task;

  int state = disable_ints ();

  // Do all tasks
  queue_reset (&_task_list);
  while ( task = queue_next (&_task_list), task != NULL ) {
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
//      kprintf ("Dec Alarma on pid %d %d!!!\n", task->pid, task->alarm);
      if (task->alarm == 0) {
        kprintf ("Alarma on pid %d !!!\n", task->pid);
        //sys_signal (task, SIGALRM);
      }
    }

    // A signal is found and the task can be interrupted. Set the task to be ready again
    if (task->signal && task->state == TASK_STATE_INTERRUPTABLE) {
      task->state = TASK_STATE_RUNNABLE;
    }
  }

  restore_ints (state);
}




/****
 *
 */
void switch_task () {
  CYBOS_TASK *previous_task;
  CYBOS_TASK *next_task;

  // There is a signal pending. Do that first before we actually run code again (TODO: is this really a good idea?)
  if (_current_task->signal != 0) return;


  int state = disable_ints ();

  // This is the task we're running. It will be the old task after this
  next_task = previous_task = _current_task;

  /* Don't use _current_task from this point on. We define previous_task as the one we are
   * about to leave, and next_task as the one we are about to enter. */


  // Only set this task to be available again if it was still running. If it's
  // sleeping TASK_STATE_(UN)INTERRUPTIBLE), then don't change this setting.
  if (previous_task->state == TASK_STATE_RUNNING) {
    previous_task->state = TASK_STATE_RUNNABLE;
  }

  // Fetch the next available task
  next_task = get_next_runnable_task ();

  // Looks like we do not need to switch (maybe only 1 task, or still idle?)
  if (previous_task == next_task) {
    kprintf ("!");
    restore_ints (state);
    return;
  }

  // Old task is available again. New task is running
  next_task->state = TASK_STATE_RUNNING;

  // Set the kernel stack
  tss_set_kernel_stack (next_task->kstack + KERNEL_STACK_SIZE);
  set_pagedirectory (next_task->page_directory);

  // The next task is now the current task..
  _current_task = next_task;

  sti ();
  context_switch (&previous_task->esp, next_task->esp);

  return;
}

/****
 * Switches to usermode. On return we are no longer in the kernel
 * but in the RING3 usermode. Make sure we have done all kernel
 * setup before this.
 */
void switch_to_usermode (void) {
  _current_task = (CYBOS_TASK *)queue_reset (&_task_list);

  // Set the kernel stack
  tss_set_kernel_stack(_current_task->kstack + KERNEL_STACK_SIZE);

  // Move from kernel ring0 to usermode ring3 AND start interrupts at the same time.
  // This is needed because we cannot use sti() inside usermode. Since there is no 'real'
  // way of reaching usermode, we have to 'jumpstart' to it by creating a stackframe which
  // we return from. When the IRET we return to the same spot, but in a different ring.
  asm volatile("  movw   %%cx, %%ds;    \n\t" \
               "  movw   %%cx, %%es;    \n\t" \
               "  movw   %%cx, %%fs;    \n\t" \
               "  movw   %%cx, %%gs;    \n\t" \

               "  movl   %%esp, %%ebx;  \n\t" \
               "  pushl  %%ecx;         \n\t" \
               "  pushl  %%ebx;         \n\t" \
               "  pushfl;               \n\t" \
               "  popl   %%eax;         \n\t" \
               "  or     $0x200, %%eax; \n\t" \
               "  pushl  %%eax;         \n\t" \
               "  pushl  %%edx;         \n\t" \
               "  pushl  $1f;           \n\t" \
               "  iret;                 \n\t" \
               "1:                      \n\t" \
               ::"d"(SEL(KUSER_CODE_DESCR, TI_GDT+RPL_RING3)),
                 "c"(SEL(KUSER_DATA_DESCR, TI_GDT+RPL_RING3)));

  // We now run in RING3 mode, with the kernel-stack in the TTS set to
  // the idle-task's stack AND with interrupts running.. We are go...
}

// ========================================================================================
int allocate_new_pid (void) {
  CYBOS_TASK *tmp;
  int b;

  int state = disable_ints ();

  do {
    b = 0;    // By default, we assume current_pid points to a free PID

    // Increase current PID or wrap around
    current_pid = (current_pid < MAX_PID) ? current_pid+1 : PID_IDLE;

    // No task list present, no need to check for running pids
    if (queue_count(&_task_list) == 0) return current_pid;

    // Check if there is a process currently running with this PID
    while (tmp = (CYBOS_TASK *)queue_next(&_task_list), tmp && b != 1) {
      if (tmp->pid == current_pid) b = 1;
    }
  } while (b == 1);


  restore_ints (state);

  return current_pid;
}

// ========================================================================================
int sys_sleep (int ms) {
  int state = disable_ints ();

  if (_current_task->pid == PID_IDLE) kpanic ("Cannot sleep idle task!");

  kprintf ("Sleeping process %d for %d ms\n", _current_task->pid, ms);

  _current_task->alarm = ms;
  _current_task->state = TASK_STATE_INTERRUPTABLE;

  restore_ints (state);

  // We're sleeping. So go to a next task.
  reschedule ();

  return 0;
}

// ========================================================================================
int sys_exit () {
  kprintf ("Sys_exit called!!!!");
  for (;;) ;
}

// ========================================================================================
int exit (void) {
  int ret;
  __asm__ __volatile__ ("int	$" SYSCALL_INT_STR " \n\t" : "=a" (ret) : "a" (SYS_EXIT) );
  return ret;
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
  __asm__ __volatile__ ("int	$" SYSCALL_INT_STR " \n\t" : "=a" (ret) : "a" (SYS_SLEEP), "b" (ms) );
  return ret;
}

// ========================================================================================
int idle (void) {
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

// kprintf ("HLT()ing system until IRQ\n");
  sti();
  hlt();
  kprintf ("!");
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

  int state = disable_ints();

  // The current task will be the parent task
  parent_task = _current_task;

  // Create a new task
  child_task = (CYBOS_TASK *)kmalloc (sizeof (CYBOS_TASK));

  // copy all data from the parent into the child
  memcpy (child_task, parent_task, sizeof (CYBOS_TASK));

  // Available for scheduling
  child_task->state = TASK_STATE_INITIALISING;

  // The page directory is the cloned space
  clone_debug = 1;
  child_task->page_directory = clone_pagedirectory (parent_task->page_directory);
  clone_debug = 0;

  child_task->pid  = allocate_new_pid ();           // Make new PID
  child_task->ppid = parent_task->pid;              // Set the parent pid

  // Reset task times for the child
  child_task->ringticksHi[0] = child_task->ringticksLo[0] = 0;
  child_task->ringticksHi[1] = child_task->ringticksLo[1] = 0;
  child_task->ringticksHi[2] = child_task->ringticksLo[2] = 0;
  child_task->ringticksHi[3] = child_task->ringticksLo[3] = 0;

  kprintf ("sys_fork() Creating kernel stack for this process\n");
  Uint32 addr;

  kprintf ("OLD KSTACK IS : %08X   Phys: %08X\n",child_task->kstack, addr);

  child_task->kstack = (Uint32)kmalloc_pageboundary_physical (KERNEL_STACK_SIZE, &addr);
  memcpy (&child_task->kstack, &parent_task->kstack, KERNEL_STACK_SIZE);

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
    Uint32 *stacktop = (Uint32 *)child_task->kstack + (KERNEL_STACK_SIZE / sizeof(Uint32));
    stacktop--;
    *stacktop = eip;

/*
    int i,j;
    j = 0xdead0000;
    for (i=0; i<8; i++) {
      stacktop--;
      *stacktop = j;
      j++;
    }
*/
    child_task->esp = (Uint32)stacktop;

    // Available for scheduling
    child_task->state = TASK_STATE_RUNNABLE;

    restore_ints (state);
    return child_task->pid;   // Return the PID of the child

  } else {
    restore_ints (state);
    return 0;     // This is the child. Return 0
  }
}


/****
 * Switches to another task and does housekeeping in the meantime
 */
void reschedule () {
  global_task_administration ();      /* Sort priorities, alarms, wake-up-on-signals etc */
  switch_task ();                     /* Switch to another task */
  handle_pending_signals ();          /* Handle pending signals for current task */
}


/**
 * Creates TSS structure in the GDT. We only need the SS0 (always the same) and ESP0
 * to point to the kernel stack. That's it.
 */
void sched_init_create_tss () {
  // Generate a TSS and set it
  memset (&tss_entry, 0, sizeof (TSS));

  // Set general TTS in the GDT. This is used for task switching (sort of)
  Uint64 tss_descriptor = gdt_create_descriptor ((int)&tss_entry, (int)sizeof (TSS), GDT_PRESENT+GDT_DPL_RING0+GDT_NOT_BUSY+GDT_TYPE_TSS, GDT_NO_GRANULARITY+GDT_AVAILABLE);
  gdt_set_descriptor (TSS_TASK_DESCR, tss_descriptor);

  // Load/flush task register, note that no entries in the TSS are filled
  __asm__ __volatile__ ( "ltrw %%ax\n\t" : : "a" (TSS_TASK_SEL));
}

/****
 * Initialises the multitasking environment by creating
 * the kernel task. After the multitasking, the function
 * loads this task by setting it's eip to the point right
 * after loading the task. This way we 'jump' to the task,
 * about the same way we 'jump' into protected mode.
 *
 */
int sched_init () {
  sched_init_create_tss ();

  // We create a task here.. Actually, we define the current task that is running. It
  // changes into the idle-task as soon as we switch to another process. Since this is the
  // first call to create_kernel_thread, we get PID 0, which is a special pid as well
  // (ie: cannot be killed, goto sleep etc). Since this is not completely a new task, we
  // do not need to define the entry_point (the entrypoint will be saved on the first
  // context-switch call. This is more or less the jump-start of context switching.
  thread_create_kernel_thread (0, "Idle process", CONSOLE_USE_KCONSOLE);

  // Set the first task (which is the idle-task) priority to idle
  CYBOS_TASK *idle_task = queue_reset (&_task_list);
  idle_task->priority = PRIO_IDLE;

  return ERR_OK;
}
