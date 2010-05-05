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
  typedef char   state_t;
  typedef Uint32 pid_t;
  typedef Uint8  prio_t;

  #pragma pack (1)
  typedef struct {
      void *prev;                             // Previous task or NULL on start
      void *next;                             // Next task or NULL on end

      char name[50];                          // Name of the current task so it shows up inside the task listing
      TCONSOLE *console;                      // Pointer to the TCONSOLE were this task is outputted to

      state_t state;                          // Status of the current task.
      prio_t priority;                        // Priority of the process   (between PRIO_LOWEST - PRIO_HIGHEST)

      Uint64 utime;                           // Time spent in ring 0 to 3 (normally, only ring 0 and 3 are used)
      Uint64 ktime;                           // Time spent in ring 0 to 3 (normally, only ring 0 and 3 are used)

      Uint32 *kstack;                         // Points to kernel stack
      Uint32 *ustack;                         // Points to user stack

      TREGS *context;

      TPAGEDIRECTORY *page_directory;         // Points to the page directory of this task

      int  alarm;                             // Remaining alarm ticks
      int  signal;                            // Current raised signals (bitfields)

      pid_t pid;                              // PID of the task
      pid_t ppid;                             // PID of the parent task (or 0 on no parent)
  } CYBOS_TASK;


  #define PRIO_LOW            1     // Minimum priority
  #define PRIO_DEFAULT       50     // Default priority
  #define PRIO_HIGH         100     // Maximum priority
  #define PRIO_TIME_SLICE    10     // Decrease X priority every tick

  #define PID_IDLE            0     // PID of the idle task (fixed)
  #define PID_INIT            1     // PID of the init task (fixed)
  #define MAX_PID         65535     // Maximum nr of pids

  #define SCHEDULE_TICKS     50     // Every 50ms there will be a context-switch

  // defines for CYBOS_TASK.state
  #define TASK_STATE_INITIALISING     'i'       // Do not schedule at this moment.
  #define TASK_STATE_RUNNABLE         'r'       // Ready for scheduling.
  #define TASK_STATE_RUNNING          'R'       // This task is currently running
  #define TASK_STATE_INTERRUPTABLE    'S'       // Task is sleeping, but can be interrupted
  #define TASK_STATE_UNINTERRUPTABLE  'U'       // Task is sleeping, and cannot be interrupted

  extern CYBOS_TASK *_current_task;             // Current task which is running.
  extern CYBOS_TASK *_idle_task;                // Idle task
  extern int current_pid;                       // Last PID returned by allocate_pid()

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

  void global_task_administration (void);
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
