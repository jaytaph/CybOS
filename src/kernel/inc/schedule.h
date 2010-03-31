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

  // Console creation defines for thread_create_*
  #define CONSOLE_USE_KCONSOLE           0    // Use the kernel console
  #define CONSOLE_CREATE_NEW             1    // Create new console
  #define CONSOLE_NO_CONSOLE             2    // Thread does not use console


  // Standard cybos task. This is basically a raw process structure

  #pragma pack (1)
  typedef struct {
      char name[50];                                  // Name of the current task so it shows up inside the task listing
      TCONSOLE *console;                              // Pointer to the TCONSOLE were this task is outputted to

      char state;                                     // Status of the current task.
      int  priority;                                  // Priority of the process
      int  ringticksHi[4];                            // Time spent in ring 0 to 3 (normally, only ring 0 and 3 are used, lower 32bits)
      int  ringticksLo[4];                            // Time spent in ring 0 to 3 (normally, only ring 0 and 3 are used, upper 32bits)

      int  kstack;                                    // Points to kernel stack
      int  ustack;                                    // Points to user stack

      Uint32 esp;                                     // Current kernel stack ESP
      TPAGEDIRECTORY *page_directory;                 // Points to the page directory of this task

      int  alarm;                                     // Remaining alarm ticks
      int  signal;                                    // Current raised signals (bitfields)

      int  pid;                                       // PID of the task
      int  ppid;                                      // PID of the parent task
  } CYBOS_TASK;


  #define PRIO_IDLE           0     // Idle priority
  #define PRIO_LOWEST         1     // Minimum priority
  #define PRIO_DEFAULT       50     // Default priority
  #define PRIO_HIGHEST      100     // Maximum priority

  #define PID_IDLE            0     // PID of the idle task
  #define MAX_PID         65535     // Maximum nr of pids



  // Defines for _create_task
  #define TASK_KERNEL         0
  #define TASK_USER           1

  // defines for CYBOS_TASK.state
  #define TASK_STATE_IDLE             'I'       // Idle task
  #define TASK_STATE_INITIALISING     'i'       // Do not schedule at this moment.
  #define TASK_STATE_RUNNABLE         'r'       // Ready for scheduling.
  #define TASK_STATE_RUNNING          'R'       // This task is currently running
  #define TASK_STATE_INTERRUPTABLE    'S'       // Task is sleeping, but can be interrupted
  #define TASK_STATE_UNINTERRUPTABLE  'U'       // Task is sleeping, and cannot be interrupted
//  #define TASK_STATE_ZOMBIE           'Z'       // Zombie task (?)

  extern CYBOS_TASK *_current_task;             // Current task which is running.
//  extern CYBOS_TASK *_task_list;                // Points to the first task in the tasklist (idle_task)
  extern int current_pid;                       // Last PID returned by allocate_pid()


  int read_eip (void);

  int sched_init (void);
  void reschedule (void);

  void user_idle (void);
  void kernel_idle (void);
  void switch_to_usermode (void);
  int allocate_new_pid (void);

  void sched_add_task (CYBOS_TASK *task);
  void sched_remove_task (CYBOS_TASK *task);
  void sched_add_runnable_task (CYBOS_TASK *task);
  void sched_remove_runnable_task (CYBOS_TASK *task);

  void thread_create_kernel_thread (Uint32 start_address, char *taskname, int console);

  int sys_fork (void);
  int fork(void);

  int getpid (void);
  int sys_get_pid (void);

  int getppid (void);
  int sys_get_ppid (void);

  int sys_sleep (int ms);
  int sleep (int ms);

  int idle (void);
  int sys_idle (void);



#endif    // __SCHEDULE_H__
