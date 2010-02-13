/******************************************************************************
 *
 *  File        : kmem.h
 *  Description : Kernel Memory Manager
 *
 *****************************************************************************/
#ifndef __HEAP_H__
#define __HEAP_H__

  #include "ktype.h"

  extern unsigned int _heap_start;
  extern unsigned int _heap_top;
  extern unsigned int _heap_size;

  int heap_init ();

  void *_heap_kmalloc (Uint32 size, int pageboundary, Uint32 *physical_address);
  void _heap_kfree (Uint32 ptr);

#endif // __HEAP_H__
