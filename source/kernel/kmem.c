/******************************************************************************
 *
 *  File        : kmem.c
 *  Description : Kernel Memory Manager
 *
 *****************************************************************************/
#include "kernel.h"
#include "errors.h"
#include "kmem.h"
#include "heap.h"


// Basically, we use kmalloc before heap is initialized and malloc afterwards. Both functions
// check if the heap is initialised and use the correct internal functions anyway.

// Memory top of the kernel
Uint32 _k_preheap_start = NULL;       // Start of the kernel heap
Uint32 _k_preheap_top = NULL;         // end of the kernel heap
Uint32 _memory_total = 0;             // total bytes of physical memory

Uint32 _preheap_start = NULL;         // 'normal' heap start
Uint32 _preheap_top   = NULL;         // 'normal' heap end


/*
 * kmalloc and kfree have 2 different "modes" of operating. One mode is before heap is
 * initialized. In that case we use the _preheap_kmalloc/kfree functions. When the heap
 * is setup, we switch the functions to the "correct" _kmalloc/_kfree. We could have this
 * done with an 'if'-statement, but this way allows us to easily add new functions (if needed)
 * without rebuilding if's to switches etc etc.. design patterns ftw.. :p
 */
void * (*_func_kmalloc)(Uint32 size, int pageboundary, Uint32 *physical_address) = &_preheap_kmalloc;   // func_kmalloc points to preheap malloc
void (*_func_kfree)(void *ptr) = &_preheap_kfree;       // func_kfree points to preheap free

Uint32 _unfreeable_kmem = 0;



/************************************************************************
 * Switches the kmalloc and kfree to the "heap" allocator
 */
void kmem_switch_malloc (void *kmalloc, void *kfree) {
  _func_kmalloc = kmalloc;
  _func_kfree = kfree;
}

/************************************************************************
 * Initialize the kernel memory manager.
 */
int kmem_init (int total_sys_memory) {
  // Set total memory
  _memory_total = total_sys_memory;

  // Number of bytes allocated by kmalloc before heap is initialized. This
  // memory cannot be freed at this moment so use wisely!
  _unfreeable_kmem = 0;

  // This will setup the heap.
  _k_preheap_start = (unsigned int)&end;    // _k_preheap_start will start immediately after the end of the code. No need to align (actually, we should maybe?)

  // Top of the heap is the start of the heap.. nothing used..
  _k_preheap_top = _k_preheap_start;

  // Heap is not yet initialized
  _preheap_start = NULL;
  _preheap_top   = NULL;

  return ERR_OK;
}





/************************************************************************
 * Internal and very bad kmalloc. Needed when we haven't initialized the
 * heap yet. Use with caution because memory cannot be freed afterwards :(
 */
void *_preheap_kmalloc (Uint32 size, int pageboundary, Uint32 *physical_address) {
  int mem_ptr;

  // Align the heaptop to the next 4KB page if needed.
  if (pageboundary == 1 && (_k_preheap_top & 0xFFFFF000)) {
    _k_preheap_top &= 0xFFFFF000;
    _k_preheap_top += 0x1000;
  }

  // Return physical address if needed
  if (physical_address != NULL) *physical_address = ((Uint32)_k_preheap_top - 0xC0000000);

  // mem_ptr is the base address of the new block
  mem_ptr = _k_preheap_top;

  // Increase the heap top
  _k_preheap_top += size;

  _unfreeable_kmem += size;

  //kprintf ("_preheap_kmalloc(): %d bytes. S: %08X   New: %08X\n", size, mem_ptr, _k_preheap_top);

  return (void *)mem_ptr;
}


/************************************************************************
 * kfree when we haven't initialized the heap yet. It cannot free anything!
 */
void _preheap_kfree (void *ptr) {
  kpanic ("preheap_kfree() called. Not available!");
}


// ==========================================================================================
// The kmalloc/kfree functions will automatically switch to the correct internal functions
// depending on the heap. Use with caution when the heap is not initialized, since we cannot
// kfree() any blocks we kmalloc()!
void *kmalloc (Uint32 size) {
  return _func_kmalloc (size, 0, NULL);
}

void *kmalloc_pageboundary (Uint32 size) {
  return _func_kmalloc (size, 1, NULL);
}

void *kmalloc_physical (Uint32 size, Uint32 *physical_address) {
  return _func_kmalloc (size, 0, physical_address);
}

void *kmalloc_pageboundary_physical (Uint32 size, Uint32 *physical_address) {
  return _func_kmalloc (size, 1, physical_address);
}


// ==========================================================================================
//
void kfree (void *ptr) {
  return _func_kfree (ptr);
}
