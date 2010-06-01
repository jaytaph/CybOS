/******************************************************************************
 *
 *  File        : timer.h
 *  Description : Timer IRQ functions. Doesn't do anything special except
 *                calling the scheduler.
 *
 *****************************************************************************/
#ifndef __TIMER_H__
#define __TIMER_H__

  int timer_interrupt (int rpl);
  void timer_init (void);

#endif // __TIMER_H__
