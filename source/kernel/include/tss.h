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
    Uint16 link, __linkh;
    Uint32 esp0;
    Uint16 ss0, __ss0h;
    Uint32 esp1;
    Uint16 ss1, __ss1h;
    Uint32 esp2;
    Uint16 ss2, __ss2h;
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
    Uint16 es, __esh;
    Uint16 cs, __csh;
    Uint16 ss, __ssh;
    Uint16 ds, __dsh;
    Uint16 fs, __fsh;
    Uint16 gs, __gsh;
    Uint16 ldtr, __ldtrh;
    Uint16 iopb_offset;
    Uint16 T;             // Only the first bit!
  } TSS;

  void tss_set_kernel_stack (Uint32 stack_address);

#endif // __TSS_H__
