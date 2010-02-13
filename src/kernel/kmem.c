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


// Basically, we use kmalloc before heap is initialised and malloc afterwards. Both functions
// check if the heap is initialised and use the correct internal functions anyway.

// Memory top of the kernel
unsigned int k_heap_start = NULL;       // Start of the kernel heap
unsigned int k_heap_top = NULL;         // end of the kernel heap
unsigned int k_memory_total = 0;        // total bytes of physical memory

unsigned int _heap_start = NULL;        // 'normal' heap start
unsigned int _heap_top   = NULL;        // 'normal' heap end


/*
 * kmalloc and kfree have 2 different "modes" of operating. One mode is before heap is
 * initialied. In that case we use the _preheap_kmalloc/kfree functions. When the heap
 * is setup, we switch the functions to the "correct" _kmalloc/_kfree. We could have this
 * done with an 'if'-statement, but this way allows us to easily add new functions (if needed)
 * without rebuilding if's to switches etc etc.. design patterns ftw.. :p
 */
void * (*_func_kmalloc)(Uint32 size, int pageboundary, Uint32 *physical_address) = &_preheap_kmalloc;   // func_kmalloc points to preheap malloc
void (*_func_kfree)(Uint32 ptr) = &_preheap_kfree;       // func_kfree points to preheap free

unsigned int _unfreeable_kmem = 0;



/************************************************************************
 * Switches the kmalloc and kfree to the "heap" allocator
 */
void kmem_init_heap (void) {
  _func_kmalloc = &_heap_kmalloc;
  _func_kfree = &_heap_kfree;
}

/************************************************************************
 * Initialize the kernel memory manager.
 */
int kmem_init (int total_sys_memory) {
  // Set total memory
  k_memory_total = total_sys_memory;

  // Number of bytes allocated by kmalloc before heap is initialised. This
  // memory cannot be freed at this moment so use wisely!
  _unfreeable_kmem = 0;

  // This will setup the heap. We grow upwards.
  k_heap_start = (unsigned int)&end;
  k_heap_top = k_heap_start;

  // Heap is not yet initialized
  _heap_start = NULL;
  _heap_top   = NULL;

  return ERR_OK;
}





/************************************************************************
 * Internal and very bad kmalloc. Needed when we haven't initialized the
 * heap yet. Use with caution because memory cannot be freed afterwards :(
 */
void *_preheap_kmalloc (Uint32 size, int pageboundary, Uint32 *physical_address) {
  int mem_ptr;

  // Align the heaptop to the next 4KB page if needed.
  if (pageboundary == 1 && (k_heap_top & 0xFFFFF000)) {
    k_heap_top &= 0xFFFFF000;
    k_heap_top += 0x1000;
  }

  // Return physical address if needed
  if (physical_address != NULL) *physical_address = ((Uint32)k_heap_top - 0xC0000000);

  // mem_ptr is the base address of the new block
  mem_ptr = k_heap_top;

  // Increase the heap top
  k_heap_top += size;

  _unfreeable_kmem += size;

//  kprintf ("_preheap_kmalloc(): %d bytes. Old: %08X   S: %08X   New: %08X\n", size, old_k_heap_top, mem_ptr, k_heap_top);

  // Why doesn't newblock->base work!?
  return (void *)mem_ptr;
}


/************************************************************************
 * kfree when we haven't initialized the heap yet. It cannot free anything!
 */
void _preheap_kfree (Uint32 ptr) {
  kprintf ("preheap_kfree() called. Not available!");
  kdeadlock ();
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
void kfree (Uint32 ptr) {
  return _func_kfree (ptr);
}
