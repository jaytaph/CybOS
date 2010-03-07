/******************************************************************************
 *
 *  File        : schedule.c
 *  Description : Schedule defines and function headers
 *
 *****************************************************************************/
#ifndef __SCHEDULE_H__
#define __SCHEDULE_H__

  #include "kernel.h"
  #include "paging.h"

  #define NO_CREATE_CONSOLE              0
  #define CREATE_CONSOLE                 1


  // Standard cybos task. This is basically a raw process structure

  #pragma pack (1)
  typedef struct {
      void *next;                                     // Pointer to the next CYBOS_TASK task in the DLL.
      void *prev;                                     // Pointer to the previous CYBOS_TASK task in the DLL.

      char name[50];                                  // Name of the current task so it shows up inside the task listing
      TCONSOLE *console;                              // Pointer to the TCONSOLE were this task is outputted to

      char state;                                     // Status of the current task.
      int  priority;                                  // Priority of the process
      int  ringticksHi[4];                            // Time spent in ring 0 to 3 (normally, only ring 0 and 3 are used, lower 32bits)
      int  ringticksLo[4];                            // Time spent in ring 0 to 3 (normally, only ring 0 and 3 are used, upper 32bits)

      int  kstack;                                    // Points to kernel stack
      int  ustack;                                    // Points to user stack

      Uint32 eip;
      Uint32 ebp;
      Uint32 esp;                                     // Current kernel stack ESP
      Uint32 ss;                                      // Current kernel stack SS (needed?)
      TPAGEDIRECTORY *page_directory;                 // Points to the page directory of this task

      int  alarm;                                     // Remaining alarm ticks
      int  signal;                                    // Current raised signals (bitfields)

      int  pid;                                       // PID of the task
      int  ppid;                                      // PID of the parent task
  } CYBOS_TASK;


  #define PRIO_DEFAULT       50     // Default priority
  #define PRIO_LOWEST         0     // Minimum priority (idle tasks)
  #define PRIO_HIGHEST      100     // Maximum priority

  #define PID_IDLE            0     // PID of the idle task
  #define MAX_PID         65535     // Maximum nr of pids



  // Defines for _create_task
  #define TASK_KERNEL         0
  #define TASK_USER           1

  // defines for CYBOS_TASK.state
  #define TASK_STATE_INITIALISING     'I'       // Do not schedule at this moment.
  #define TASK_STATE_RUNNABLE         'r'       // Ready for scheduling.
  #define TASK_STATE_RUNNING          'R'       // This task is currently running
  #define TASK_STATE_INTERRUPTABLE    'S'       // Task is sleeping, but can be interrupted
  #define TASK_STATE_UNINTERRUPTABLE  'U'       // Task is sleeping, and cannot be interrupted
  #define TASK_STATE_ZOMBIE           'Z'       // Zombie task (?)

//  extern unsigned char charstates[];          // Array of chars with 'task state numbers'

  extern CYBOS_TASK *_current_task;            // Current task which is running.
  extern CYBOS_TASK *_task_list;               // Points to the first task in the tasklist (idle_task)
  extern int current_pid;                      // Last PID returned by allocate_pid()

//  extern unsigned int *_kernel_stack;          // Pointer to the kernel stack


  int read_eip (void);

  int sched_init (void);
  void scheduler (void);

  void user_idle (void);
  void kernel_idle (void);
  void switch_to_usermode (void);
  int allocate_new_pid (void);

  void sched_add_task (CYBOS_TASK *task);
  void sched_remove_task (CYBOS_TASK *task);

  int sched_create_kernel_task (CYBOS_TASK *task, Uint32 eip, char *taskname, int console);
  int sched_create_user_task (CYBOS_TASK *task, Uint32 eip, char *taskname, int console);

  int sys_fork (void);
  int sys_sleep (int ms);

  int sleep (int ms);


#endif    // __SCHEDULE_H__
