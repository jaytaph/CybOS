/******************************************************************************
 *
 *  File        : service.c
 *  Description : Service Routine like DOS 0x21 interrupt
 *
 *****************************************************************************/
#include "kernel.h"
#include "schedule.h"

Uint32 _schedule_ticks = 0;

/************************************************************
 * Interrupt handler for IRQ 0. Control speed through the
 * PIT-functions in pit.h
 */
int timer_interrupt (int rpl) {
  // Increase the first character on the screen so we can see we are running
#ifdef __DEBUG__
  char *vga = (char *)0xF00B8000;
  (*vga) = rpl+'0';

  vga+=2; (*vga)++;
  if (*vga==0) { vga+=2; (*vga)++; }
#endif

  // Increase main kernel timer tick counter
  _kernel_ticks++;

  // Nothing left to do when we do not have tasks initialized yet
  if (! _current_task) return 0;

  // Increase the time spend on this ring for this process
  _current_task->ringticksLo[rpl]++;
  if (_current_task->ringticksLo[rpl] == 0) {
    _current_task->ringticksHi[rpl]++;
  }

/*
  _schedule_ticks++;
  if (_schedule_ticks > 50) {
    _schedule_ticks = 0;
    return 1;
  }
  return 0;
*/

  return 1;
}

