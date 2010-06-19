/******************************************************************************
 *
 *  File        : tss.c
 *  Description : TSS routines and managment.
 *
 *****************************************************************************/

#include "kernel.h"

/**
 * Sets the stack address in the TSS. As soon as the CPU makes a switch to
 * ring 0, it will use these SS:ESP values (sets it and pushes all interrupt data
 * onto it)
 */
void tss_set_kernel_stack (Uint32 stack_address) {
  kprintf ("TSS Set Kernel Stack %08X\n", stack_address);

  // Get descriptor that holds main TSS
  Uint64 tss_descriptor = gdt_get_descriptor (TSS_TASK_DESCR);

  // Get base address of TSS
  TSS *tss_base = (TSS *)gdt_get_base (tss_descriptor);

  // Set SS0 (static) and stack_address into correct TTS registers
  tss_base->ss0 = SEL(KERNEL_DATA_DESCR, TI_GDT+RPL_RING0);
  tss_base->esp0 = stack_address;
}