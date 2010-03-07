/******************************************************************************
 *
 *  File        : service.c
 *  Description : Service Routine like DOS 0x21 interrupt
 *
 *****************************************************************************/
#include "kernel.h"
#include "schedule.h"

/************************************************************
 * Interrupt handler for IRQ 0. Control speed through the
 * PIT-functions in pit.h
 */
void timer_interrupt (int rpl) {
  // Increase the first character on the screen so we can see we are running
#ifdef __DEBUG__
  char *vga = (char *)0xF00B8000;
  (*vga) = rpl+65;

  vga+=2; (*vga)++;
  if (*vga==0) { vga+=2; (*vga)++; }
#endif

  // Increase main kernel timer tick counter
  // @TODO: 32 bit or 64 bits counter?
  _kernel_ticks++;

  // Nothing left to do when we do not have tasks initialized yet
  if (! _current_task) return;

  // Increase the time spend on this ring for this process
  _current_task->ringticksLo[rpl]++;
  if (_current_task->ringticksLo[rpl] == 0) {
    _current_task->ringticksHi[rpl]++;
  }

  // TODO: Why not schedule() on every tick?
  if (_kernel_ticks & 1) scheduler ();
}

