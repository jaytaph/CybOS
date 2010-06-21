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

int sys_exec (char *path) {
  kprintf ("sys_executing '%s'\n", path);

  load_binary_elf (path);
  return 0;
}
