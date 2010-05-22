/******************************************************************************
 *
 *  File        : main.c
 *  Description : Main startup file. Must be linked as the first entry in order
 *                to keep the kernel entry point at a relative address of 0.
 *
 *****************************************************************************/
#include "kernel.h"

/************************************************************
 * We created this wrapper because it's in a seperate object which we can link in front of the
 * rest. This is to make sure that this function stays at 0x0000000 in .TEXT. The main-kernel
 * function in kernel.c doesn't do that because of all the data inside the functions.
 */
void kernel_entry_point (int stack_start, int total_sys_memory, char *boot_params) {
  // Just call the *REAL* kernel_entry point from kernel.c. That's it.
  kernel_entry (stack_start, total_sys_memory, boot_params);

  // Deadlock, we should never be here anyway. and we cannot return from this function since
  // there is a dummy return value on the stack
  for (;;) ;
}
