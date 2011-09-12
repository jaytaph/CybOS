/******************************************************************************
 *
 *  File        : exec.h
 *  Description : Execute functionality
 *
 *****************************************************************************/

#ifndef __EXEC_H__
#define __EXEC_H__

extern char **environ;

  unsigned int sys_execve (regs_t *r, char *path, char **args, char **environ);
  int execve (char *path, char **args, char **environ);

#endif //__EXEC_H__
