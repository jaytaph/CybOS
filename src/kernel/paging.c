/******************************************************************************
 *
 *  File        : paging.c
 *  Description :
 *
 *
 *****************************************************************************/
#include "errors.h"
#include "kernel.h"
#include "kmem.h"
#include "idt.h"
#include "gdt.h"
#include "paging.h"

bitmap_t *framebitmap;


// Kernel stack
unsigned int *_kernel_stack;


unsigned int clone_debug = 0;


pagedirectory_t *clone_pagedirectory (pagedirectory_t *src);
void copy_physical_pageframe_data (Uint32 src, Uint32 dst);

/************************************************************
 * Let processor use the new page directory
 */
void set_pagedirectory (pagedirectory_t *pagedir) {
  __asm__ __volatile__ ("movl %%eax, %%cr3  \n\t" : : "a" (pagedir->physical_address));
}

// Flushes the cache. This will allow the CPU to see the updates in the pagedir
void flush_pagedirectory (void) {
  Uint32 tmp;
  __asm__ __volatile__ ("movl %%cr3, %0 \n\t" \
                        "movl %0, %%cr3 \n\t" \
                        : "=r" (tmp) );
}


/************************************************************
 * Do a pagefault (not really handled yet)
 */
void do_page_fault (regs_t *r) {
  Uint32 cr2;
  __asm__ __volatile__ ("mov %%cr2, %0" : "=r" (cr2));

  int present = !(r->err_code & 0x1);
  int rw  = r->err_code & 0x2;
  int us  = r->err_code & 0x4;
  int res = r->err_code & 0x8;
  int id  = r->err_code & 0x10;

  kprintf ("CYBOS Page fault handler (");
  if (present) { kprintf ("present,"); } else { kprintf ("not present,"); }
  if (rw) { kprintf ("read/write,"); } else { kprintf ("read only,"); }
  if (us) { kprintf ("user-mode,"); } else { kprintf ("supervisor mode,"); }
  if (res) { kprintf ("reserved,"); } else { kprintf ("not reserved,"); }
  if (id) { kprintf ("id"); } else { kprintf ("no id"); }
  kprintf (") at 0x%08X\n", cr2);

  kdeadlock ();
}



// ====================================================================================
// Bit set/clear/set routines
void bm_set (bitmap_t *bitmap, int frame_address) {
  int index = IFB (bitmap, frame_address);
  int offset = OFB (bitmap, frame_address);
  bitmap->map[index] |= (1 << offset);
}
void bm_clear (bitmap_t *bitmap, int frame_address) {
  int index = IFB (bitmap, frame_address);
  int offset = OFB (bitmap, frame_address);
  bitmap->map[index] &= ~(1 << offset);

  // We cleared a bit that is lower then our FF cache. Move the pointer
  if (index < bitmap->firstempty) bitmap->firstempty = index;
}
int bm_test (bitmap_t *bitmap, int frame_address) {
  int index = IFB (bitmap, frame_address);
  int offset = OFB (bitmap, frame_address);
  return (bitmap->map[index] & (1 << offset));
}

// ====================================================================================
// Find the first free 0 bit in the bitmap
int bm_findfirst (bitmap_t *bitmap) {
  int i,j;

  int sf = (bitmap->firstempty < 0) ? 0 : bitmap->firstempty;

  for (i=sf; i!=bitmap->size; i++) {
    if (bitmap->map[i] != -1) {
      for (j=0; j!=bitmap->bitsize; j++) {
        int testbit = 1 << j;
        if ( ! (bitmap->map[i] & testbit)) {
          bitmap->firstempty = i-1;
          return (i * bitmap->bitsize + j);
        }
      }
    }
  }

  // No free pages left :(
  return -1;
}


// ====================================================================================
// Find a new empty page in the bitmap if this page is not initialized. Set the page flags as well.
void allocate_pageframe (page_t *page, int rw, int level) {
  int index;

  // There is already a address present (== allocated)
  if (*page != 0) return;

  index = bm_findfirst (framebitmap);

  if (index == -1) kpanic ("Out of free pages. Note: should swap memory to disc here.. ?");

  // Set this frame as marked inside the bitmap
  bm_set (framebitmap, index);

  // Set info in the page
  *page = index << 12;
  *page |= PAGEFLAG_PRESENT;
  if (rw) *page |= PAGEFLAG_READWRITE;
  if (level) *page |= PAGEFLAG_USER;
}

// ====================================================================================
void obsolete_free_frame (page_t *page) {
  // Frame was not allocated in the first place
  if (*page != 0) return;

  // Clear frame in bitmap
  bm_clear (framebitmap, (*page >> 12) );

  // Remove address from frame
  *page = 0;
}



/*
  PAGING

    pre-paging
      0x0 is mapped to 0xC0000000 for X megabyte (enough for kernel + heap)
      create frame bitmap
      mark all area's: bios stuff, vga, kernel (code + heap)

      howto create the pagedir ?
      howto heap to 0xD0000000
      howto stack to 0xE0000000
      howto normal <16MB mem to 0xF0000000 (for DMA, 0xB800 etc)

    post-paging
      stack on 0xE00000
      heap on 0xD00000
      kernel prints on 0xF00B8000
*/




// ====================================================================================
pagedirectory_t *create_pagedirectory (void) {
  Uint32 tmp;

  // Allocate the kernel page directory on page boundary
  pagedirectory_t *pagedir = (pagedirectory_t *) kmalloc_pageboundary_physical (sizeof (pagedirectory_t), &tmp);

  // Clear the whole structure
  memset (pagedir, 0, sizeof (pagedirectory_t));

  // Note that this is the START of the structure. This means we need the phystables[] to be at the START of the structure as well.
  // Another way would be to have 2 separate structures in here. One for maintenance and one for CR3.
  pagedir->physical_address = tmp;

  return pagedir;
}



// ====================================================================================
pagedirectory_t *clone_pagedirectory (pagedirectory_t *src) {
  pagedirectory_t *dst;
  int i,j;
  Uint32 phys_addr;

  // Allocate the kernel page directory and clear the whole structure
  dst = (pagedirectory_t *) kmalloc_pageboundary_physical (sizeof (pagedirectory_t), &phys_addr);
  memset (dst, 0, sizeof (pagedirectory_t));     // Zero it out
  dst->physical_address = phys_addr;            // We need to know the physical address of the pagedir so we can load it into CR3 register

// kprintf ("\n** Cloning page directory to %08X\n", phys_addr);

  if (clone_debug) {
    kprintf ("But first... some information about the SRC table...\n");
    kprintf ("physical_address: %08X\n", src->physical_address);
    for (i=0; i!=1024; i++) {
      if (src->phystables[i] != 0) {
        char c;
        c = ((src->phystables[i] & 0xFFFFF000) != (_kernel_pagedirectory->phystables[i] & 0xFFFFF000)) ? 'C' : 'L';

        kprintf ("TABLE[%4d] [%c]  phys addr : %08X\n", i, c, src->phystables[i]);
        int k =  (i == 832) ? 25 : 5;
        for (j=0; j!=k; j++) {
          if (src->tables[i]->pages[j] != 0) {
            Uint32 va = (i << 22) + (j << 12);
            kprintf ("     PAGE[%d] : %08X  (%08X)\n", j, src->tables[i]->pages[j], va);
          }
        }
      }
    }
  }


  for (i=0; i!=1024; i++) {
    if (src->tables[i] == 0) continue;    // Don't copy zero table

//    kprintf (" clone_pd(): phystables[%d] = %08X\n", i, src->phystables[i]);

    if ((src->phystables[i] & 0xFFFFF000) != (_kernel_pagedirectory->phystables[i] & 0xFFFFF000)) {
//      kprintf (" [C] %08X\n", (src->phystables[i] & 0xFFFFF000));
      // COPY the table since it's not in the kernel directory

      // Create table at new memory address
      dst->tables[i] = (pagetable_t *)kmalloc_pageboundary_physical (sizeof (pagetable_t), &phys_addr);
      dst->phystables[i] = phys_addr | 0x7;

      // Make sure the new table is clear
      memset (dst->tables[i], 0, sizeof (pagetable_t));
      for (j=0; j!=1024; j++) {
        if (src->tables[i]->pages[j] == 0) continue;    // Empty frame, don't copy

//          kprintf ("Copying page %d (%08X)\n", j, src->tables[i]->pages[j]);

        // Create new page
        allocate_pageframe (&dst->tables[i]->pages[j], 0, 0);

        // Copy pageflag bits from source to destination
        if (src->tables[i]->pages[j] & PAGEFLAG_PRESENT)   dst->tables[i]->pages[j] |= PAGEFLAG_PRESENT;
        if (src->tables[i]->pages[j] & PAGEFLAG_READWRITE) dst->tables[i]->pages[j] |= PAGEFLAG_READWRITE;
        if (src->tables[i]->pages[j] & PAGEFLAG_USER)      dst->tables[i]->pages[j] |= PAGEFLAG_USER;
        if (src->tables[i]->pages[j] & PAGEFLAG_ACCESSED)  dst->tables[i]->pages[j] |= PAGEFLAG_ACCESSED;
        if (src->tables[i]->pages[j] & PAGEFLAG_DIRTY)     dst->tables[i]->pages[j] |= PAGEFLAG_DIRTY;

        // @TODO: We know both virtual addresses don't we? If not, we just create a place where we
        // can put these 2 items and copy them over.
        // We need to copy the physical frame data. TODO: Don't use 0xF0000000, but map these 2 frames
        // to anywhere so we know their virtual location. Copy them and unmap again.
        // copy_physical_pageframe_data(src->phystables[i] & 0xFFFFF000, dst->phystables[i] & 0xFFFFF000);

        copy_physical_pageframe_data (src->tables[i]->pages[j] & 0xFFFFF000, dst->tables[i]->pages[j] & 0xFFFFF000);
      }
    } else {
      // LINK the table since it's also in the kernel directory
//      kprintf (" [L] %08X\n", (src->phystables[i] & 0xFFFFF000));

      // Since we just link to the same address, we do not need to copy the table itself.
      dst->phystables[i] = src->phystables[i];
      dst->tables[i] = src->tables[i];
    }
  }

//kprintf ("done...\n");

  return dst;
}



void create_pageframe (pagedirectory_t *directory, Uint32 dst_address, int pagelevels) {
  Uint32 dst_frame, dst_table, dst_page, tmp;

//  kprintf ("Find a page for destination: 0x%08X\n", dst_address);
  dst_frame = dst_address / 0x1000;

  dst_table = dst_frame / 1024;
  dst_page  = dst_frame % 1024;

  // Create table if it does not exist.
  if (directory->tables[dst_table] == NULL) {
    // Allocate and save physical memory offset so CR3 can find this
    directory->tables[dst_table] = (pagetable_t *)kmalloc_pageboundary_physical (sizeof (pagetable_t), &tmp);
    directory->phystables[dst_table] = tmp | 0x7;

    memset (directory->tables[dst_table], 0, sizeof (pagetable_t));

//    kprintf ("CPG Creating phystables[%d] = %08X\n", dst_table, tmp);

  }

  // The page for this address is already present
  if (directory->tables[dst_table]->pages[dst_page] != NULL) return;


  Uint32 frame_index = bm_findfirst (framebitmap);
  if (frame_index == -1) kpanic ("Out of frames in framebitmap!");  // TODO: Should swap here?
  bm_set (framebitmap, frame_index);

  directory->tables[dst_table]->pages[dst_page] = (frame_index * 0x1000) | pagelevels;

//  kprintf ("Frame index found: 0x%05X 000\n", frame_index);
}

// ====================================================================================
/*
 * Return the PAGE structure for a certain address. This depends on the directory used.
 * When no page structure is available (ie the table is not yet created), the create_flag
 * decide if the table should be made in the directory or not.
 */
void map_virtual_memory (pagedirectory_t *directory, Uint32 src_address, Uint32 dst_address, int pagelevels, int set_bitmap) {
  Uint32 tmp, src_frame, dst_frame, dst_table, dst_page;


//  kprintf ("map_virtual_memory 0x%08X -> 0x%08X\n", src_address, dst_address);

  // Is the page directory already initialized?
  if (directory->physical_address == NULL) return;

  // Get the frame from the address
  src_frame = src_address / 0x1000;
  dst_frame = dst_address / 0x1000;

  dst_table = dst_frame / 1024;    // Find which table this frame is in.
  dst_page  = dst_frame % 1024;    // Find the page of this frame in the table

//  kprintf ("DST_ADDR : %08X\n", dst_address);
//  kprintf ("DST_FRAME: %08X\n", dst_frame);
//  kprintf ("DST_TABLE: %08X\n", dst_table);
//  kprintf ("DST_PAGE : %08X\n", dst_page);

  // Create table if it does not exist.
  if (directory->tables[dst_table] == NULL) {
    // Allocate and save physical memory offset so CR3 can find this
    directory->tables[dst_table] = (pagetable_t *)kmalloc_pageboundary_physical (sizeof (pagetable_t), &tmp);

    // Zero out the new pagetable
    memset (directory->tables[dst_table], 0, sizeof (pagetable_t));

    // Store physical address (+ flags)
//    kprintf ("MVM Creating phystables[%d] = %08X\n", dst_table, tmp);
    directory->phystables[dst_table] = tmp | 0x7;
  }

  // Set this frame to point to the source frame
  directory->tables[dst_table]->pages[dst_page] = ((src_frame * 0x1000) | pagelevels);

  // Set this bit to 1, don't care if it's already set. It's just for initialization anyway so we assume it's 0
  if (set_bitmap == SET_BITMAP) bm_set (framebitmap, src_frame);
}


// ====================================================================================
void pbm (int size) {
  int i,j;
  kprintf ("Bitmap:\n");

  char s[40];
  for (j=0; j!=size; j++) {
    for (i=0; i!=32; i++) {
      if ((framebitmap->map[j] & (1 << i)) == 0) {
        s[i] = '0';
      } else {
        s[i] = '1';
      }
    }
    s[32] = ' ';
    s[33] = '\0';
    kprintf ("%s", s);
    if (j % 3 == 2) kprintf ("\n");
  }

  kprintf ("\n\n");
}


// ====================================================================================
// Fetch the page address from the directory, and add the rest (first 12 bits of the address) to
// get the physical address.
Uint32 get_physical_address (pagedirectory_t *directory, Uint32 virtual_address) {
  Uint32 frame = virtual_address / 0x1000;
  Uint32 table = frame / 1024;
  Uint32 page  = frame % 1024;

  return (directory->tables[table]->pages[page] & 0xFFFFF000) + (virtual_address & 0xFFF);
}


// ====================================================================================
int stack_init (Uint32 src_stack_top) {
  Uint32 old_esp, old_ebp;
  Uint32 new_esp, new_ebp;
  Uint32 stacklength, kernel_stack_top;
  int i;
/*
  // Allocate room for a new stack
  // @TODO: THIS ALREADY WORKS, BUT WE USE 0xCF000000 for easy debugging!
  _kernel_stack = (unsigned int *)kmalloc (KERNEL_STACK_SIZE);
//  kprintf ("\nKernel stack based on %08X\n", _kernel_stack);

//  // TODO: IS IT REALLY A KERNEL-STACK??? IS IT!???? YOU SURE!???
  _kernel_stack = (unsigned int *)0xCF000000;

  // Allocate some space for the new stack.
  for(i=0; i< KERNEL_STACK_SIZE; i += 0x1000) {
    create_pageframe (_current_pagedirectory, 0xCF000000+i, PAGEFLAG_PRESENT+PAGEFLAG_READWRITE+PAGEFLAG_USER);
  }
*/

  // Allocate room for a new stack.

  // We cannot use kmalloc because we need to make sure our stack starts in a new directory-table and not inside
  // a directory-page. This is needed because the clone_pagedirectory only checks for differences in the
  // tables, not the pages inside those tables.
  _kernel_stack = (unsigned int *)0xCF000000;
  for(i=0; i < KERNEL_STACK_SIZE; i += 0x1000) {
    create_pageframe (_current_pagedirectory, (Uint32)_kernel_stack + i, PAGEFLAG_PRESENT+PAGEFLAG_READWRITE+PAGEFLAG_USER);
  }

  // Work with 2 stack-tops instead of kernel-stack-bottom and src-stack-bottom
  kernel_stack_top = (Uint32)_kernel_stack + KERNEL_STACK_SIZE;

  // Fetch current stack pointers
  __asm__ __volatile__ ("mov %%esp, %0" : "=r" (old_esp));
  __asm__ __volatile__ ("mov %%ebp, %0" : "=r" (old_ebp));

  // This is the length of the stack. Should be around 100 bytes more or less
  stacklength = src_stack_top - old_esp;

  // Calculate the new pointers
  new_esp = (Uint32)kernel_stack_top - (src_stack_top - old_esp);
  new_ebp = (Uint32)kernel_stack_top - (src_stack_top - old_ebp);

  // Copy stack, but only until the current stack pointer. Maybe you notice it (or not), but we
  // also push some extra stuff on the stack while copying (namely the parameters for memcpy). These
  // get copyied too, but since we saved the orinal ESP, these values are useless and get
  // overwritten later on. We don't care about that..
  memcpy ((void *)(kernel_stack_top-stacklength), (void *)(src_stack_top-stacklength), stacklength);

  // Set the new ESP and EBP. Note that EBP might not be valid at the moment but we can't be sure.
  __asm__ __volatile__ ("mov %0, %%esp" : : "r" (new_esp));
  __asm__ __volatile__ ("mov %0, %%ebp" : : "r" (new_ebp));

  flush_pagedirectory ();

  return ERR_OK;
}



// ====================================================================================
int paging_init () {
  int i, framecount;

  // Allocate a bitmap big enough to hold 1 bit for each page of memory
  framecount = _memory_total / 0x1000;                       // We use 4KB pages. Framecount is the number of frames of PHYSICAL memory
  framebitmap = (bitmap_t *)kmalloc (sizeof (bitmap_t));        // This hold the allocated pages in each bit
  framebitmap->bitsize = 32;                                  // We use chars in our map (means 8 bits per "index")
  framebitmap->size = framecount / framebitmap->bitsize;      // Keep the size. Might come in handy later.
  framebitmap->map = (Uint32 *)kmalloc (framebitmap->size);   // Now allocate memory for the map
  memset (framebitmap->map, 0, framebitmap->size);            // And clear all bits


  // Let's setup paging from scratch. We are still running on 0xC0000000, which we should not change obviously. We create a new
  // page directory that maps 0x0 to 0xC0000000. We should stop at: end of kernel data + kernel heap (from kmalloc). This is
  // located in the k_heap_top variable. The 0xD00000 will be the heap, 0xE00000 something else and 0xF0000000 will be mapped
  // 1:1 to the lowest 16MB of the physical memory (don't even care if we do not have so much). It's used for direct BIOS access
  // and DMA stuff later on.

  _kernel_pagedirectory = create_pagedirectory ();

  // Actually, we should start at the beginning of the kernel (.text), not start of memory, but alas
  i = (unsigned int)0xC0000000;

  // TODO: 0xC00FFFFF.. is this enough?
  while (i < 0xC00FFFFF) {    // k_heap_top will change during this run. No for() loops or constants!
    // @TODO: Change this to PAGEFLAG_KERNEL WHEN READY (!?)
    map_virtual_memory (_kernel_pagedirectory, (i - 0xC0000000), (i - 0xC0000000), PAGEFLAG_USER | PAGEFLAG_PRESENT | PAGEFLAG_READWRITE, SET_BITMAP);
    map_virtual_memory (_kernel_pagedirectory, (i - 0xC0000000), i, PAGEFLAG_USER | PAGEFLAG_PRESENT | PAGEFLAG_READWRITE, SET_BITMAP);
    i += 0x1000;
  }

  // Manually have to set some BIOS stuff in the bitmap that cannot be used by the memory allocator.
  bm_set (framebitmap, 0x0);  // Page 0 (0x0000 - 0x0FFF)
  for (i=0xA0; i<=0xFF; i++) bm_set (framebitmap, i); // 0xA0000 - 0xFFFFF

  // On 0xF0000000 we map the 1:1 the lower 16Mb (even if we do not have so much), 0xF00B8000 therefore should point to vga vidmem. We do not reserve
  // the room in the bitmap since that would mean the allocator cannot use the whole low 16MB.
  for (i=0; i < (16*1024*1024); i+= 0x1000) {
    map_virtual_memory (_kernel_pagedirectory, i, (i + 0xF0000000), PAGEFLAG_USER | PAGEFLAG_PRESENT | PAGEFLAG_READWRITE, DONT_SET_BITMAP);
  }

  // We mapped all important kernel area's. Later on, we add a heap, stack and more stuff.
  set_pagedirectory (_kernel_pagedirectory);

  return ERR_OK;
}
