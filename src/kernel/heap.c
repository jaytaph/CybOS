/******************************************************************************
 *
 *  File        : heap.c
 *  Description : Heap thingies
 *
 *****************************************************************************/
#include "kernel.h"
#include "heap.h"
#include "kmem.h"
#include "errors.h"
#include "paging.h"

#define HEAP_START          0xD0000000      // Start of the heap
#define MIN_HEAP_SIZE       0x50000         // Initial and also minimal heap size
#define MIN_HEAP_GROWSIZE   0x10000         // Grow everytime with this size
#define HEAPMAGIC           0xCAFEBABE


typedef struct {
  Uint32 magic;
  Uint8 is_hole;
  Uint32 size;
} THEAPHEADER;

typedef struct {
  Uint32 magic;
  THEAPHEADER *header;
} THEAPFOOTER;

typedef struct {
  int index;
  Uint32 start;
  Uint32 end;
  Uint32 max;
  Uint8 supervisor;     // TODO: why not 1 byte with pageflags?
  Uint8 readonly;
} THEAP;


  Uint32 _k_heap_start;
  Uint32 _k_heap_top;
  Uint32 _k_heap_end;
  Uint32 _k_heap_size;


/************************************************************************
 * Initialize the kernel memory manager.
 */
int heap_init (void) {
  int i;

  // Set temporary heap settings
  Uint32 tmp_k_heap_start = HEAP_START;
  Uint32 tmp_k_heap_top   = tmp_k_heap_start;
  Uint32 tmp_k_heap_end   = tmp_k_heap_start + MIN_HEAP_SIZE;
  Uint32 tmp_k_heap_size  = tmp_k_heap_end - tmp_k_heap_start;

  // Here we allocate the memory for the heap.
  for (i=tmp_k_heap_start; i!=tmp_k_heap_end; i+=0x1000) {
    // Create a new frame in the _kernel_pagedirectory at a unknown place.
//    kprintf ("Creating heap frame: %08X\n", i);
    create_pageframe (_kernel_pagedirectory, i, PAGEFLAG_USER + PAGEFLAG_PRESENT + PAGEFLAG_READWRITE);
    // TODO: create_pageframe (_kernel_pagedirectory, i, PAGEFLAG_SUPERVISOR + PAGEFLAG_PRESENT + PAGEFLAG_READWRITE);
  }

  // We just added the 0xD0000000 stuff. Flush pagedirectory
  flush_pagedirectory ();

  // We MUST set _k_heap_start AFTER allocating memory for the heap.
  _k_heap_start = tmp_k_heap_start;
  _k_heap_top   = tmp_k_heap_top;
  _k_heap_end   = tmp_k_heap_end;
  _k_heap_size  = tmp_k_heap_size;

  // Switch to new malloc system
  kmem_switch_malloc (_heap_kmalloc, _heap_kfree);

  return ERR_OK;
}

void heap_expand (Uint32 size) {
//  kprintf ("Expanding heap from 0x%08X with %d bytes.\n", _k_heap_size, size);

  // TODO: add new page frames?
  _k_heap_end += size;
}

void heap_shrink (Uint32 size) {
//  kprintf ("Shrinking heap back from 0x%08X with %d bytes.\n", _k_heap_size, size);

  // TODO: remove page frames?
  _k_heap_end -= size;
}


// ================================================================================
// Same as _kmalloc, but only allocate inside the HEAP. There is no way to get a
// physical address back from malloc().
void *_heap_kmalloc (Uint32 size, int pageboundary, Uint32 *physical_address) {
  int mem_ptr;

  // Expand the heap if needed
  while (_k_heap_top + size > _k_heap_end) {
    kprintf ("Expanding heap\n");
    heap_expand (MIN_HEAP_GROWSIZE);
  }

//  int old_k_heap_top = _k_heap_top;

  // Align the heaptop to the next 4KB page if needed.
  if (pageboundary == 1 && (_k_heap_top & 0xFFFFF000)) {
    _k_heap_top &= 0xFFFFF000;
    _k_heap_top += 0x1000;
  }

  // mem_ptr is the base address of the new block
  mem_ptr = _k_heap_top;


  // We now the virtual address, we can lookup the physical address in the _kernel_pagedirectory
  if (physical_address != NULL) {
    (*physical_address) = get_physical_address (_kernel_pagedirectory, mem_ptr);
//    kprintf ("_kmalloc(): phys addr: %08X --> %08X\n", mem_ptr, *physical_address);
  }

  // Increase the heap top
  _k_heap_top += size;

//kprintf ("_kmalloc(): Allocated %d bytes. Ptr: %08X   New: %08X\n", size, mem_ptr, _k_heap_top);
  return (void *)mem_ptr;
}



// ========================================================
// Frees a block
void _heap_kfree (Uint32 ptr) {
  kprintf ("kfree() not implemented yet.");
}


