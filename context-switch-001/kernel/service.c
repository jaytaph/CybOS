/******************************************************************************
 *
 *  File        : service.c
 *  Description : Service Routine like DOS 0x21 interrupt
 *
 *****************************************************************************/
#include "kernel.h"
#include "service.h"
#include "conio.h"
#include "console.h"
#include "schedule.h"
#include "keyboard.h"


// These are all corresponding service calls
create_syscall_entry0(exit,    SYS_EXIT)
create_syscall_entry0(getppid, SYS_GETPPID)
create_syscall_entry0(getpid,  SYS_GETPID)
create_syscall_entry0(fork,    SYS_FORK)
create_syscall_entry0(idle,    SYS_IDLE)
create_syscall_entry0(signal,  SYS_SIGNAL)
create_syscall_entry1(sleep,   SYS_SLEEP,  ms)



// Could be handled by a lowlevel lookup-table, but this is probably a bit more readable
  int service_interrupt (int sysnr, int p1, int p2, int p3, int p4, int p5) {
    int retval;

    switch (sysnr) {
      case  SYS_NULL :
                      retval = sys_null ();
                      break;
      case  SYS_CONSOLE :
                      retval = sys_console (p1, (TCONSOLE *)p2, (char *)p3);
                      break;
      case  SYS_CONWRITE :
                      retval = sys_conwrite ((char)p1, p2);
                      break;
      case  SYS_CONFLUSH :
                      retval = sys_conflush ();
                      break;
      case  SYS_FORK :
                      retval = sys_fork ();
                      break;
      case  SYS_GETPID :
                      retval = sys_getpid ();
                      break;
      case  SYS_GETPPID :
                      retval = sys_getppid ();
                      break;
      case  SYS_SLEEP :
                      retval = sys_sleep (p1);
                      break;
      case  SYS_IDLE :
                      retval = sys_idle ();
                      break;
      case  SYS_EXIT :
                      retval = sys_exit ();
                      break;
    }
    return retval;
  }


  // ========================================================
  int sys_null (void) {
    return 0;
  }

  // ========================================================
  int sys_conwrite (char ch, int autoflush) {
    con_putch (_current_task->console, ch);
    if (autoflush) sys_conflush ();
    return 0;
  }

  // ========================================================
  int sys_conread (void) {
    return keyboard_poll ();
  }

  // ========================================================
  int sys_conflush (void) {
    con_flush (_current_task->console);
    return 0;
  }
