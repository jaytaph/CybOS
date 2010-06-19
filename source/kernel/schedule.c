/******************************************************************************
 *
 *  File        : idt.c
 *  Description : Interrupt routines and managment.
 *
 *****************************************************************************/
#include "kernel.h"
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


task_t *_current_task = NULL;    // Current active task on the CPU.
task_t *_task_list = NULL;       // Points to the first task in the tasklist (which should be the idle task)
task_t *_idle_task;              // Points to the idle task (which should be the first task in _task_list)

TSS tss_entry;                   // Memory to hold the TSS. Address is loaded into the GDT

int current_pid = PID_IDLE - 1;      // First call to allocate_pid will return PID_IDLE


void context_switch (Uint32 *prev_context, Uint32 new_context);    // Found in task.S
Uint32 read_eip (void);



/**
 * Creates a new thread (process)
 */
void sched_init_pid_0 () {
  // Create room for task
  task_t *task = (task_t *)kmalloc (sizeof (task_t));

  // We are initialising this task at the moment
  task->state = TASK_STATE_INITIALISING;

  // Create kernel stack and setup "start" stack that gets pop'ed by context_switch()
  task->kstack = (Uint32 *)kmalloc_pageboundary (KERNEL_STACK_SIZE);
  task->context = (regs_t *)(task->kstack + KERNEL_STACK_SIZE - sizeof (regs_t));
  task->esp0 = (Uint32)task->kstack + KERNEL_STACK_SIZE;

  // Set page directory
  task->page_directory = _current_pagedirectory;

  // PID's + priority
  task->priority = PRIO_DEFAULT;
  task->pid = allocate_new_pid ();
  task->ppid = 0;

  // Name of the task
  strncpy (task->name, "Idle process", 49);

  // Times spend in each CPL ring (only 0 and 3 are used)
  task->ktime = task->utime = 0;

  // Use the kernel console
  task->console = _kconsole;

  // Add task to schedule-switcher. We are still initialising so it does not run yet.
  sched_add_task (task);

  // We are all done. Available for scheduling
  task->state = TASK_STATE_RUNNABLE;
}



/*******************************************************
 * Adds a task to the linked list of tasks.
 */
void sched_add_task (task_t *new_task) {
  int state = disable_ints ();

  task_t *last_task = _task_list;

  // No tasks found, this task will be the first. This is only true when adding the primary task (idle-task).
  if (last_task == NULL) {
    _task_list = new_task;  // Let task_list point to the first task
    new_task->next = NULL;  // No next tasks in the list
    new_task->prev = NULL;  // And also no previous tasks in the list

    _idle_task = new_task;  // Also set idle_task pointer, used for readability

  } else {
    // Find last task in the list
    while (last_task->next != NULL) last_task = last_task->next;

    // Add to the list
    last_task->next = new_task;       // End of link points to the new task
    if (last_task != new_task) {
      new_task->prev = last_task;     // Create a link back if it's not the first task
    }
    new_task->next = NULL;            // And the new task points to the end of line..
  }

  // Enable ints (if needed)
  restore_ints (state);
}

/*******************************************************
 * Removes a task from the linked list of tasks.
 */
void sched_remove_task (task_t *task) {
  task_t *tmp,*tmp1;

  // Don't remove idle_task!
  if (task->pid == PID_IDLE) kpanic ("Cannot delete idle task!");

  // Disable ints
  int state = disable_ints ();

  // Simple remove when it's the last one on the list.
  if (task->next == NULL) {
    tmp = (task_t *)task->prev;
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

  // Enable ints (if needed)
  restore_ints (state);
}


/*******************************************************
 * Fetch the next RUNNABLE task from the list, OR NULL when no runnable process is found
 */
task_t *get_next_runnable_task (task_t *current_task) {
  task_t *next_task = NULL;

  // Disable ints
  int state = disable_ints ();

  // There is only 1 process available, the idle task. Special case in testing this
  if (current_task->prev == NULL && current_task->next == NULL) {
     next_task = current_task;
     goto found;
  }

  // Find the next runnable task (this can never be the idle-task!)
  next_task = current_task;
  do {
    // Goto next task or wrap around (to the SECOND task so we skip idle-task)
    next_task = (next_task->next != NULL) ? next_task->next : _task_list;

// @TODO: REMOVE ME FAST!!! THIS WILL SELECT THE IDLE TASK AS WELL!
//    if (next_task->state == TASK_STATE_RUNNABLE && next_task->pid != PID_IDLE) goto found;
    if (next_task->state == TASK_STATE_RUNNABLE) goto found;
  } while (next_task != current_task);

  // No runnable tasks found. return NULL so the scheduler can pick up the idle-task
  next_task = NULL;

found:
  // Enable ints (if needed)
  restore_ints (state);

  return next_task;
}



/****
 *
 */
void sys_signal (task_t *task, int signal) {
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
  task_t *task;

  int state = disable_ints ();

  // Do all tasks
  for (task = _task_list; task != NULL; task = task->next) {
    // This task is not yet ready. Don't do anything with it
    if (task->state == TASK_STATE_INITIALISING) continue;

    // Don't adjust idle task
    if (task->pid == PID_IDLE) continue;

    // Check for alarm. Decrease alarm and send a signal if time is up
    if (task->alarm) {
      task->alarm--;
      if (task->alarm == 0) sys_signal (task, SIGALRM);
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
  task_t *previous_task;
  task_t *next_task;

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
  next_task = get_next_runnable_task (previous_task);

  // No task found? Use the idle-task
  if (next_task == NULL) next_task = _idle_task;

  // Looks like we do not need to switch (maybe only 1 task, or still idle?)
  if (previous_task == next_task) {
    restore_ints (state);
    return;
  }

  kprintf ("Reschedule from PID %d to PID %d\n", previous_task->pid, next_task->pid);

  // Old task is available again. New task is running
  if (next_task->pid != PID_IDLE) {
    next_task->state = TASK_STATE_RUNNING;
  }



  // Set kernel stack and page directroy of new task
  tss_set_kernel_stack ((Uint32)(next_task->kstack + KERNEL_STACK_SIZE));
  set_pagedirectory (next_task->page_directory);

  Uint32 esp, ebp, eip;
  asm volatile("mov %%esp, %0" : "=r"(esp));
  asm volatile("mov %%ebp, %0" : "=r"(ebp));

  eip = read_eip();
  if (eip == 0x12345) return;

  previous_task->esp = esp;
  previous_task->ebp = ebp;
  previous_task->eip = eip;

  eip = next_task->eip;
  esp = next_task->esp;
  ebp = next_task->ebp;

  // The next task is now the current task..
  _current_task = next_task;

  kprintf ("Jumping to %08x [ESP: %08X]\n", eip, esp);

BOCHS_BREAKPOINT;
  asm volatile("         \
    cli;                 \
    mov %0, %%ecx;       \
    mov %1, %%esp;       \
    mov %2, %%ebp;       \
    mov $0x12345, %%eax; \
    sti;                 \
    jmp *%%ecx           "
                 : : "r"(eip), "r"(esp), "r"(ebp));
}

/****
 * Switches to usermode. On return we are no longer in the kernel
 * but in the RING3 usermode. Make sure we have done all kernel
 * setup before this.
 */
void switch_to_usermode (void) {

  /* Current task is the start of the task_list, which points to our idle task which we
   * have initialized during sched_init() */
  _current_task = _task_list;

  // Set the correct kernel stack
  tss_set_kernel_stack((Uint32)(_current_task->kstack + KERNEL_STACK_SIZE));

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
  task_t *tmp;
  int b;

  int state = disable_ints ();

  do {
    b = 0;    // By default, we assume current_pid points to a free PID

    // Increase current PID or wrap around
    current_pid = (current_pid < MAX_PID) ? current_pid+1 : PID_IDLE;

    // No task list present, no need to check for running pids
    if (_task_list == NULL) return current_pid;

    // Check if there is a process currently running with this PID
    for (tmp = _task_list; tmp->next != NULL && b != 1; tmp = tmp->next) {
      if (tmp->pid == current_pid) b = 1;
    }
  } while (b == 1);


  restore_ints (state);

  return current_pid;
}




/**
 * Ring0 idle mode. Can only be called by the idle task
 */
int sys_idle () {
  /* We can only idle when we are the 0 (idle) task. Other tasks cannot idle, because if
   * they are doing nothing (for instance, they are waiting on a signal (sleep, IO etc), they
   * should not idle themself, but other processes will take over (this is taken care of by
   * the scheduler). So anyway, when there is absolutely no process available that can be
   * run (everybody is waiting), the scheduler will choose the idle task, which only job is
   * to call this function (over and over again). It will put the processor into a standby
   * mode and waits until a IRQ arrives. It's way better to call a HLT() than to do a
   * "for(;;) ;".. */
  if (_current_task->pid != PID_IDLE) {
    kpanic ("Idle() called by a PID > 0...\n");
    return -1;
  }

  sti();
  hlt();
  return 0;
}


/**
 *
 */
int sys_sleep (int ms) {
  int state = disable_ints ();

  if (_current_task->pid == PID_IDLE) kpanic ("Cannot sleep idle task!");

  kprintf ("Sleeping process %d for %d ms\n", _current_task->pid, ms);

  _current_task->alarm = ms;
  _current_task->state = TASK_STATE_INTERRUPTABLE;

  restore_ints (state);

  kprintf ("Reschedule()\n");

  // We're sleeping. So go to a next task.
  reschedule ();

  return 0;
}


/**
 * Exits current task
 */
int sys_exit () {
  if (_current_task->pid == PID_IDLE) {
    kpanic ("Exit() called by a PID > 0...\n");
    return -1;
  }

  // @TODO: Free all user-allocated pages from the page-directory
  // @TODO: Remove task from list and free task

  kprintf ("Sys_exit called!!!!");
  for (;;) ;
}


/**
 * Returns the current process ID
 */
int sys_getpid (void) {
  return _current_task->pid;
}


/**
 * Returns the current parent process ID
 */
int sys_getppid (void) {
  return _current_task->ppid;
}


/**
 * Forks the current process
 */
int sys_fork (regs_t *r) {
  task_t *child_task, *parent_task;

  int state = disable_ints();

  // The current task will be the parent task
  parent_task = _current_task;

  // Create a new task
  child_task = (task_t *)kmalloc (sizeof (task_t));

  // copy all data from the parent into the child
  memcpy (child_task, parent_task, sizeof (task_t));

  // Available for scheduling
  child_task->state = TASK_STATE_RUNNABLE;

  // The page directory is the cloned space
  clone_debug = 0;
  child_task->page_directory = clone_pagedirectory (parent_task->page_directory);
  clone_debug = 0;

  child_task->pid  = allocate_new_pid ();           // Make new PID
  child_task->ppid = parent_task->pid;              // Set the parent pid

  // Reset task times for the child
  child_task->ktime = child_task->utime = 0;

  // Allocate child's kernel stack
  child_task->kstack = (Uint32 *)kmalloc_pageboundary (KERNEL_STACK_SIZE);
  kprintf ("KMalloced chidl task at %08X\n", (Uint32)child_task->kstack);

  sched_add_task (child_task);

  Uint32 eip = read_eip ();
  if (_current_task == parent_task) {
    Uint32 esp;  asm volatile ("mov %%esp, %0" : "=r" (esp));
    Uint32 ebp;  asm volatile ("mov %%ebp, %0" : "=r" (ebp));
    child_task->esp = esp;
    child_task->ebp = ebp;
    child_task->eip = eip;

    restore_ints (state);
    return child_task->pid;
  }

  restore_ints (state);
  return 0;


/*
  memcpy ((void *)child_task->kstack, (void *)parent_task->kstack, KERNEL_STACK_SIZE);

  Uint32 *ptr = (Uint32 *)(child_task->kstack + (KERNEL_STACK_SIZE/4));
  int i;
  for (i=0; i!=20; i++) {
    kprintf ("CHILD %d %08X %08X\n", i, ptr, *ptr);
    ptr--;
  }

  ptr = (Uint32 *)(parent_task->kstack + (KERNEL_STACK_SIZE/4));
  for (i=0; i!=20; i++) {
    kprintf ("PARENT %d %08X %08X\n", i, ptr, *ptr);
    ptr--;
  }

//  child_task->context = (regs_t *)(child_task->kstack + KERNEL_STACK_SIZE - sizeof (regs_t));
//  child_task->esp0 = (Uint32)child_task->kstack + KERNEL_STACK_SIZE;

  // User stack gets copied by the clone_pagedirectory() function

//  // Allocate child's kernel stack and copy parent stack over
//  child_task->kstack = (Uint32 *)kmalloc_pageboundary (KERNEL_STACK_SIZE);
//  memcpy ((void *)child_task->kstack, (void *)parent_task->kstack, KERNEL_STACK_SIZE);

//  child_task->context->eax = 0xDEADBEEF;
//  parent_task->context->eax = 0xCAFEBABE;

//  // Set returning EAX value
//  regs_t *regs = (regs_t *)(child_task->kstack + KERNEL_STACK_SIZE - sizeof (regs_t));
//  regs->eax = 0xDEADBEEF;

  // Add task to schedule-switcher. We are still initialising so it does not run yet.
  sched_add_task (child_task);

  kprintf (" -- Task list --\n");
  task_t *task;
  for (task = _task_list; task != NULL; task = task->next) {
    kprintf ("%02d  %c  %-20s\n", task->pid, task->state, task->name);
  }
  kprintf (" ---------------\n\n");

  restore_ints (state);
  return child_task->pid;   // Return the PID of the child
*/
}


/**
 * Switches to another task and does housekeeping in the meantime
 */
void reschedule () {
  switch_task ();                     // Switch to another task
  handle_pending_signals ();          // Handle pending signals for current task
}


/**
 * Creates TSS structure in the GDT. We only need the SS0 (always the same) and ESP0
 * to point to the kernel stack. That's it. Needed because the i386 uses this when
 * moving from ring3 to ring0.
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


/**
 * Initialises the multitasking environment by creating
 * the kernel task. After the multitasking, the function
 * loads this task by setting it's eip to the point right
 * after loading the task. This way we 'jump' to the task,
 * about the same way we 'jump' into protected mode.
 *
 */
int sched_init () {
  sched_init_create_tss ();

  /* Setup task for PID 0. The first switch will save everything into task 0 so we don't
   * need to jumpstart to a certain entrypoint */
  sched_init_pid_0 ();

//  /* We create a task here.. Actually, we define the current task that is running. It
//   * changes into the idle-task as soon as we switch to another process. Since this is the
//   * first call to create_kernel_thread, we get PID 0, which is a special pid as well
//   * (ie: cannot be killed, goto sleep etc). Since this is not completely a new task, we
//   * do not need to define the entry_point (the entrypoint, amongst all other things will
//   * be saved on the first context-switch call. This is more or less the jump-start for
//   * context switching. */
//  thread_create_kernel_thread (0, "Idle process", CONSOLE_USE_KCONSOLE);

  // @TODO: Fix this everywhere
  return ERR_OK;
}
