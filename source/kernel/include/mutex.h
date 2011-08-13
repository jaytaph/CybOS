/******************************************************************************
 *
 *  File        : mutex.h
 *  Description : Mutual Exclusion header functions
 *
 *****************************************************************************/
#ifndef __MUTEX_H__
#define __MUTEX_H__

  int mutex_lock (int *mutex);
  int mutex_unlock (int *mutex);

#endif //__MUTEX_H__
