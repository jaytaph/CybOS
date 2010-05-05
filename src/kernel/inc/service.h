/******************************************************************************
 *
 *  File        : service.h
 *  Description : Service Routine like DOS 0x21 interrupt header and defines
 *
 *****************************************************************************/
#ifndef __SERVICE_H__
#define __SERVICE_H__

  #include "idt.h"
  #include "console.h"
  #include "schedule.h"


  // Syscall defines
  #define SYS_NULL                        0
  #define SYS_CONSOLE                     1
      #define SYS_CONSOLE_CREATE               0
      #define SYS_CONSOLE_DESTROY              1
  #define SYS_CONWRITE                    2
  #define SYS_CONREAD                     3
  #define SYS_CONFLUSH                    4

  #define SYS_FORK                       10
  #define SYS_SLEEP                      11
  #define SYS_GETPID                     12
  #define SYS_GETPPID                    13
  #define SYS_IDLE                       14
  #define SYS_EXIT                       15
  #define SYS_SIGNAL                     16


  /* Function macro's to define syscall functions. Bascially every syscall get's a special syscall function. For instance:
   * the sys_exit() syscall function (which only can get called from kernel mode), gets a exit() function which can get called
   * from usermode. It does nothing more than calling the correct sys_* function with parameters. Since all these functions
   * are similiar, we create a nice little macro for them so we don't have to type them all. The number after the macro name
   * is the number of arguments that the function can handle (most of them will be 0, but some may be 1 or 2) */
  #define create_syscall_entry0(func,syscall_func)  \
                             int func (void) {      \
                               int ret;             \
                               __asm__ __volatile__ ("int $" SYSCALL_INT_STR " \n\t" : "=a" (ret) : "a" (syscall_func) );  \
                               return ret;          \
                             }
  #define create_syscall_entry1(func,syscall_func,arg1)  \
                             int func (int arg1) {      \
                               int ret;             \
                               __asm__ __volatile__ ("int $" SYSCALL_INT_STR " \n\t" : "=a" (ret) : "a" (syscall_func), "b" (arg1) );  \
                               return ret;          \
                             }
  #define create_syscall_entry2(func,syscall_func,arg1,arg2)  \
                             int func (int arg1, int arg2) {      \
                               int ret;             \
                               __asm__ __volatile__ ("int $" SYSCALL_INT_STR " \n\t" : "=a" (ret) : "a" (syscall_func), "b" (arg1), "c" (arg2) );  \
                               return ret;          \
                             }


  int service_interrupt (int sysnr, int p1, int p2, int p3, int p4, int p5);


  int sys_null (void);
  int sys_console (int subcommand, TCONSOLE *console, char *name);
  int sys_conwrite (char ch, int autoflush);
  int sys_conread (void);
  int sys_conflush (void);
  int sys_getpid (void);
  int sys_getppid (void);
  int sys_idle (void);
  int sys_exit (void);

#endif //__SERVICE_H__
