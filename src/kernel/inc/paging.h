/******************************************************************************
 *
 *  File        : paging.h
 *  Description : Defines for the paging system.
 *
 *****************************************************************************/
#ifndef __PAGING_H__
#define __PAGING_H__

  typedef Uint32 TPAGE;

  typedef struct {
    TPAGE pages[1024];
  } TPAGETABLE;

  typedef struct page_directory {
    Uint32 phystables[1024];             // Physical address of the tables THIS HAS GOT THE BE THE FIRST ENTRY IN THIS STRUCTURE!
    int physical_address;                // Physical address of this structure

    TPAGETABLE *tables[1024];            // The tables itself, phystables points to these tables
  } TPAGEDIRECTORY;

  typedef struct bitmap {
    int size;         // Size (in index, not bits)
    int bitsize;      // how many bits in each "index"
    Uint32 *map;      // The map itself
    int firstempty;   // Points to the first non-complete map
  } TBITMAP;


  extern unsigned int clone_debug;


  void set_pagedirectory (TPAGEDIRECTORY *pagedir);
  void get_page (TPAGEDIRECTORY *directory, Uint32 dst_address, int pagelevels);
  void flush_pagedirectory (void);
  void create_pageframe (TPAGEDIRECTORY *directory, Uint32 dst_address, int pagelevels);
  Uint32 get_physical_address (TPAGEDIRECTORY *directory, Uint32 virtual_address);
  TPAGEDIRECTORY *clone_pagedirectory (TPAGEDIRECTORY *src);


  TPAGEDIRECTORY *_kernel_pagedirectory;      // Page directory for kernel
  TPAGEDIRECTORY *_current_pagedirectory;     // Currently used pagedirectory

  // Page flags
  #define PAGEFLAG_NOT_PRESENT       0x00
  #define PAGEFLAG_PRESENT           0x01

  #define PAGEFLAG_READONLY          0x00
  #define PAGEFLAG_READWRITE         0x02

  #define PAGEFLAG_SUPERVISOR        0x00
  #define PAGEFLAG_USER              0x04

  #define PAGEFLAG_NOT_ACCESSED      0x00
  #define PAGEFLAG_ACCESSED          0x20

  #define PAGEFLAG_CLEAN             0x00
  #define PAGEFLAG_DIRTY             0x40


  #define DONT_CREATE_PAGE           0
  #define CREATE_PAGE                1

  #define DONT_SET_BITMAP            0
  #define SET_BITMAP                 1


  // #define USER_STACK_SIZE        0x8000      // Initial user stack size
  // #define KERNEL_STACK_SIZE      0x1000      // Initial kernel stack size
  #define USER_STACK_SIZE         0x1200      // Initial user stack size
  #define KERNEL_STACK_SIZE       0x1200      // Initial kernel stack size (MUST BE > 0x1000 otherwise clone_pagetable does not work!)

  TBITMAP *framebitmap;       // Bitmap of physical computer memory for allocated frames

  #define IFB(bm, a) (a / bm->bitsize)
  #define OFB(bm, a) (a % bm->bitsize)

  int stack_init (Uint32 src_stack_top);
  int paging_init (void);
  void do_page_fault (TREGS *r);

#endif //__PAGING_H__


