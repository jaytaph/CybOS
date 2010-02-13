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

#define MIN_HEAP_SIZE       0x50000         // Initial and also minimal heap size
#define MIN_HEAP_GROWSIZE   0x10000         // Grow everytime with this size
#define HEAPMAGIC           0x42BEEF42


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



  unsigned int _heap_start;
  unsigned int _heap_cur;
  unsigned int _heap_end;
  unsigned int _heap_size;


/************************************************************************
 * Initialize the kernel memory manager.
 */
int heap_init (void) {
  int i;

  // Set temporary heap settings
  Uint32 tmp_heap_start = 0xD0000000;
  Uint32 tmp_heap_cur   = 0xD0000000;
  Uint32 tmp_heap_end   = 0xD0000000 + MIN_HEAP_SIZE;
  Uint32 tmp_heap_size  = tmp_heap_end - tmp_heap_start;


  // Here we allocate the memory for the heap.
  for (i=tmp_heap_start; i!=tmp_heap_end; i+=0x1000) {
    // Create a new frame in the _kernel_pagedirectory at a unknown place.
//    kprintf ("Creating heap frame: %08X\n", i);
    create_pageframe (_kernel_pagedirectory, i, PAGEFLAG_USER + PAGEFLAG_PRESENT + PAGEFLAG_READWRITE);
    // TODO: create_pageframe (_kernel_pagedirectory, i, PAGEFLAG_SUPERVISOR + PAGEFLAG_PRESENT + PAGEFLAG_READWRITE);
  }

  // We just added the 0xD0000000 stuff. Flush pagedirectory
  flush_pagedirectory ();

  // We MUST set _heap_start AFTER allocating memory for the heap.
  _heap_start = tmp_heap_start;
  _heap_cur   = tmp_heap_cur;
  _heap_end   = tmp_heap_end;
  _heap_size  = tmp_heap_size;

  return ERR_OK;
}

void heap_expand (Uint32 size) {
  kprintf ("Expanding heap from 0x%08X with %d bytes.\n", _heap_size, size);
  _heap_end += size;
}

void heap_shrink (Uint32 size) {
  kprintf ("Shrinking heap back from 0x%08X with %d bytes.\n", _heap_size, size);
  _heap_end -= size;
}


// ================================================================================
// Same as _kmalloc, but only allocate inside the HEAP. There is no way to get a
// physical address back from malloc().
void *_heap_kmalloc (Uint32 size, int pageboundary, Uint32 *physical_address) {
  int mem_ptr;

  // Expand the heap if needed
  while (_heap_cur + size > _heap_end) {
    kprintf ("Expanding heap\n");
    heap_expand (MIN_HEAP_GROWSIZE);
  }

//  int old_heap_cur = _heap_cur;

  // Align the heaptop to the next 4KB page if needed.
  if (pageboundary == 1 && (_heap_cur & 0xFFFFF000)) {
    _heap_cur &= 0xFFFFF000;
    _heap_cur += 0x1000;
  }

  // mem_ptr is the base address of the new block
  mem_ptr = _heap_cur;


  // We now the virtual address, we can lookup the physical address in the _kernel_pagedirectory
  if (physical_address != NULL) {
//    kprintf ("_kmalloc(): phys addr: %08X\n", mem_ptr);
    *physical_address = get_physical_address (_kernel_pagedirectory, mem_ptr);
  }

  // Increase the heap top
  _heap_cur += size;

//  kprintf ("_kmalloc(): Allocated %d bytes. Old: %08X   S: %08X   New: %08X\n", size, old_heap_cur, mem_ptr, _heap_cur);

  return (void *)mem_ptr;
}



// ========================================================
// Frees a block
void _heap_kfree (Uint32 ptr) {
  kprintf ("kfree() not implemented yet.");
}


