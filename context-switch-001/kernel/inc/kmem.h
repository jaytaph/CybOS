/******************************************************************************
 *
 *  File        : kmem.h
 *  Description : Kernel Memory Manager
 *
 *****************************************************************************/
#ifndef __KMEM_H__
#define __KMEM_H__

  #include "ktype.h"

  extern Uint32 _k_preheap_start;
  extern Uint32 _k_preheap_top;
  extern Uint32 _memory_total;
  extern Uint32 _unfreeable_kmem;

  int kmem_init (int total_sys_memory);

  // Method of switching the current kmalloc() and kfree() functions
  void kmem_switch_malloc (void *kmalloc_func, void *kfree_func);

  // Static memory allocate functions. After heap init, this data can be freed. Before that, it can't
  void *kmalloc (Uint32 size);
  void *kmalloc_pageboundary (Uint32 size);
  void *kmalloc_physical (Uint32 size, Uint32 *physical_address);
  void *kmalloc_pageboundary_physical (Uint32 size, Uint32 *physical_address);

  void kfree (Uint32 block);

  void _preheap_kfree (Uint32);
  void *_preheap_kmalloc (Uint32 size, int pageboundary, Uint32 *physical_address);

  // Fast memory move function
  void *kmemmove (void *dest, void *src, int n);

#endif // __KMEM_H__
