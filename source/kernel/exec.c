/******************************************************************************
 *
 *  File        : exec.c
 *  Description : Exec() functionality
 *
 *
 *****************************************************************************/
#include "kernel.h"
#include "exec.h"
#include "ff/elf.h"

char *__env[1] = { 0 };
char **environ = __env;


/**
 *
 */
int sys_execve (regs_t *r, char *path, char **args, char **environ) {
  Uint32 entrypoint = load_binary_elf (path);
  if (! entrypoint) return 0;

//  kprintf ("Entry point: %08X\n", entrypoint);
  
  // @TODO: Set arguments and set environment in the environment for this process (I guess)

/*
  // Get calling stack frame
  // @TODO: This stuff is not needed. We manipulate the stack as given by the ISR
  Uint32 stackpointer_offset = (Uint32)_current_task->kstack + KERNEL_STACK_SIZE - (Uint32)r;
  regs_t *context = (regs_t *) ((Uint32)_current_task->kstack + KERNEL_STACK_SIZE - stackpointer_offset);
*/

  /* We return to the start of the newly loaded program instead of after execve. Since we can never
   * return from an execve() since the calling EIP is removed from the stack */
  r->eip = entrypoint;
  return 0;
}
