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

int sys_execve (char *path, char **args, char **environ) {
  kprintf ("sys_executing '%s'\n", path);

  Uint32 entry = load_binary_elf (path);
  if (! entry) return -1;

  // @TODO Stack must be changed so the EIP points to the entrypoint or something
  return 0;
}
