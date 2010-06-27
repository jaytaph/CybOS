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
  #define SYS_EXECVE                     17


  /* Function macro's to define syscall functions. Bascially every syscall get's a special syscall function. For instance:
   * the sys_exit() syscall function (which only can get called from kernel mode), gets a exit() function which can get called
   * from usermode. It does nothing more than calling the correct sys_* function with parameters. Since all these functions
   * are similiar, we create a nice little macro for them so we don't have to type them all. The number after the macro name
   * is the number of arguments that the function can handle (most of them will be 0, but some may be 1 or 2) */
  #define CREATE_SYSCALL_ENTRY0(func,syscall_func)  \
                             int func (void) {      \
                               int ret;             \
                               __asm__ __volatile__ ("int $" SYSCALL_INT_STR " \n\t" : "=a" (ret) : "a" (syscall_func) );  \
                               return ret;          \
                             }
  #define CREATE_SYSCALL_ENTRY1(func,syscall_func,ARG1)  \
                             int func (ARG1 arg1) {      \
                               int ret;             \
                               __asm__ __volatile__ ("int $" SYSCALL_INT_STR " \n\t" : "=a" (ret) : "a" (syscall_func), "b" (arg1) );  \
                               return ret;          \
                             }
  #define CREATE_SYSCALL_ENTRY2(func,syscall_func,ARG1,ARG2)  \
                             int func (ARG1 arg1, ARG2 arg2) {      \
                               int ret;             \
                               __asm__ __volatile__ ("int $" SYSCALL_INT_STR " \n\t" : "=a" (ret) : "a" (syscall_func), "b" (arg1), "c" (arg2) );  \
                               return ret;          \
                             }
  #define CREATE_SYSCALL_ENTRY3(func,syscall_func,ARG1,ARG2,ARG3)  \
                             int func (ARG1 arg1, ARG2 arg2, ARG3 arg3) {      \
                               int ret;             \
                               __asm__ __volatile__ ("int $" SYSCALL_INT_STR " \n\t" : "=a" (ret) : "a" (syscall_func), "b" (arg1), "c" (arg2), "d" (arg3) );  \
                               return ret;          \
                             }
  #define CREATE_SYSCALL_ENTRY4(func,syscall_func,ARG1,ARG2,ARG3,ARG4)  \
                             int func (ARG1 arg1, ARG2 arg2, ARG3 arg3, ARG4 arg4) {      \
                               int ret;             \
                               __asm__ __volatile__ ("int $" SYSCALL_INT_STR " \n\t" : "=a" (ret) : "a" (syscall_func), "b" (arg1), "c" (arg2), "d" (arg3), "D" (arg4) );  \
                               return ret;          \
                             }
  #define CREATE_SYSCALL_ENTRY5(func,syscall_func,ARG1,ARG2,ARG3,ARG4,ARG5)  \
                             int func (ARG1 arg1, ARG2 arg2, ARG3 arg3, ARG4 arg4, ARG5 arg5) {      \
                               int ret;             \
                               __asm__ __volatile__ ("int $" SYSCALL_INT_STR " \n\t" : "=a" (ret) : "a" (syscall_func), "b" (arg1), "c" (arg2), "d" (arg3), "D" (arg4), "S" (arg5) );  \
                               return ret;          \
                             }

  int service_interrupt (regs_t *r);


  int sys_null (void);
  int sys_console (int subcommand, console_t *console, char *name);
  int sys_conwrite (char ch, int autoflush);
  int sys_conread (void);
  int sys_conflush (void);
  int sys_getpid (void);
  int sys_getppid (void);
  int sys_idle (void);
  int sys_exit (char exitcode);

#endif //__SERVICE_H__
