/******************************************************************************
 *
 *  File        : kmem.h
 *  Description : Kernel Memory Manager
 *
 *****************************************************************************/
#ifndef __KMEM_H__
#define __KMEM_H__

  #include "ktype.h"

  extern unsigned int k_heap_start;
  extern unsigned int k_heap_top;
  extern unsigned int k_memory_total;
  extern unsigned int _unfreeable_kmem;

  int kmem_init (int total_sys_memory);

  void kmem_init_heap ();

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
