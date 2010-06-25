/******************************************************************************
 *
 *  File        : exec.h
 *  Description : Execute functionality
 *
 *****************************************************************************/

#ifndef __EXEC_H__
#define __EXEC_H__

extern char **environ;

  int sys_execve (char *path, char **args, char **environ);
  int execve (char *path, char **args, char **environ);

#endif //__EXEC_H__
