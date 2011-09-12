/******************************************************************************
 *
 *  File        : kmem.h
 *  Description : Kernel Memory Manager
 *
 *****************************************************************************/
#ifndef __HEAP_H__
#define __HEAP_H__

  #include "ktype.h"

  Uint32 _k_heap_start;
  Uint32 _k_heap_top;
  Uint32 _k_heap_end;
  Uint32 _k_heap_size;

  unsigned int heap_init ();

  void *_heap_kmalloc (Uint32 size, int pageboundary, Uint32 *physical_address);
  void _heap_kfree (void *ptr);

#endif // __HEAP_H__
