/******************************************************************************
 *
 *  File        : service.c
 *  Description : Service Routine like DOS 0x21 interrupt
 *
 *****************************************************************************/
#include "kernel.h"
#include "schedule.h"

// Convert BCD number to binary. As taken from linux-0.0.1
#define BCD_TO_BIN(val) ((val)=((val)&15) + ((val)>>4)*10)

// Number of ticks each process may use (before forced scheduling to another process)
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

/**
 *
 */
void timer_init (void) {
  Uint8 d,m,y,h,i,s;

  // Wait for RTC to be in sync
  do { outb (0x70, 10); } while (inb (0x71) & 0x80);

  // Read RTC data
  outb (0x70, 0); s = inb (0x71);   // Seconds
  outb (0x70, 2); i = inb (0x71);   // Minutes
  outb (0x70, 4); h = inb (0x71);   // Hours
  outb (0x70, 7); d = inb (0x71);   // Day of month
  outb (0x70, 8); m = inb (0x71);   // Month
  outb (0x70, 9); y = inb (0x71);   // Year

  // Decode from BCD to binary
  BCD_TO_BIN (s);
  BCD_TO_BIN (i);
  BCD_TO_BIN (h);
  BCD_TO_BIN (d);
  BCD_TO_BIN (m);
  BCD_TO_BIN (y);

  // @TODO: Do something with this info
}

