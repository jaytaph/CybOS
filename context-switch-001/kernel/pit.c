/******************************************************************************
 *
 *  File        : pit.c
 *  Description : Programmable Interval Timer functions
 *
 *****************************************************************************/
#include "errors.h"
#include "kernel.h"
#include "pit.h"
#include "io.h"

/****************************************************
 * Sets the frequency of the timer (irq0) to 'frequency' hertz.
 * Use '0' to restore to normal (18.2) mode. Note that the ONLY
 * count value for which there is an whole number of ticks per
 * second is: 59.659.
 * Frequency can range between 18.2Hz and 1.29Mhz
 *
 * In:  count_value = frequency of timer in Hz.
 */
int pit_set_frequency (float count_value) {
  int i;

  // Calculate the number we need to send to the PIT
  i = PIT_FREQUENCY / count_value;

  // PIT mode is always mode 2.
  outb (PIT_CONTROL_WORD, PIT_CTR0+PIT_LSBMSB+PIT_MODE2+PIT_B16);
  outb (PIT_CHANNEL0, (i % 256));
  outb (PIT_CHANNEL0, (i / 256));

  return ERR_OK;
}

