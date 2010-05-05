/******************************************************************************
 *
 *  File        : service.c
 *  Description : Service Routine like DOS 0x21 interrupt
 *
 *****************************************************************************/
#include "kernel.h"
#include "schedule.h"

Uint32 _schedule_ticks = SCHEDULE_TICKS;

/**
 * Interrupt handler for IRQ 0. Control speed through the
 * PIT-functions in pit.h
 */
int timer_interrupt (int rpl) {
#ifdef __DEBUG__
  // Increase the first character on the screen so we can see we are running
  char *vga = (char *)0xF00B8000;
  (*vga) = rpl+'0';     // Display 0-3 on first char so we see which RPL we are in

  // More debug data
  vga+=2; (*vga)++;
  if (*vga==0) { vga+=2; (*vga)++; }
#endif

  // Increase main kernel timer tick counter
  _kernel_ticks++;

  // Nothing left to do when we do not have tasks initialized yet
  if (! _current_task) return 0;

  // Increase the time spend on this ring for this process
  if (rpl == 0) {
    _current_task->ktime++;
  } else {
    _current_task->utime++;
  }

  global_task_administration ();       // Sort priorities, alarms, wake-up-on-signals etc

  // See if it's time for a rescheduling
  _schedule_ticks--;
  if (_schedule_ticks <= 0) {
    // Time to reschedule()
    _schedule_ticks = SCHEDULE_TICKS;   // Reset schedule ticks again
    return 1;                           // Returning 1 triggers a reschedule in IRQ handler
  }

  // No reschedule
  return 0;
}

