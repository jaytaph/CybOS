/******************************************************************************
 *
 *  File        : tss.h
 *  Description : Task State Segment defines
 *
 *
 *****************************************************************************/
#ifndef __TSS_H__
#define __TSS_H__


  #pragma pack(1)
  typedef struct {
    Uint32 link;
    Uint32 esp0;
    Uint32 ss0;           // last 16 reserved
    Uint32 esp1;
    Uint32 ss1;           // last 16 reserved
    Uint32 esp2;
    Uint32 ss2;           // last 16 reserved
    Uint32 cr3;
    Uint32 eip;
    Uint32 eflags;
    Uint32 eax;
    Uint32 ebx;
    Uint32 ecx;
    Uint32 edx;
    Uint32 esp;
    Uint32 ebp;
    Uint32 esi;
    Uint32 edi;
    Uint32 es;            // last 16 reserved
    Uint32 cs;            // last 16 reserved
    Uint32 ss;            // last 16 reserved
    Uint32 ds;            // last 16 reserved
    Uint32 fs;            // last 16 reserved
    Uint32 gs;            // last 16 reserved
    Uint32 ldtr;          // last 16 reserved
    Uint16 iopb_offset;
    Uint16 T;             // Only the first bit!
  } TSS;


  void tss_mark_busy (int selector, int busy);

#endif // __TSS_H__
