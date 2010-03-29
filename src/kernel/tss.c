/******************************************************************************
 *
 *  File        : tss.c
 *  Description : TSS routines and managment.
 *
 *****************************************************************************/

#include "kernel.h"


void tss_set_kernel_stack (Uint32 stack_address) {
  Uint64 tss_descriptor = gdt_get_descriptor (TSS_TASK_DESCR);

//  kprintf ("Switching tss.esp0 to %08X\n", stack_address);

  TSS *tss_base = (TSS *)gdt_get_base (tss_descriptor);
  tss_base->ss0 = SEL(KERNEL_DATA_DESCR, TI_GDT+RPL_RING0);
  tss_base->esp0 = stack_address;
}