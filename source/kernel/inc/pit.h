/******************************************************************************
 *
 *  File        : pit.h
 *  Description : Programmable Interval Timer function header and defines
 *
 *****************************************************************************/
#ifndef __PIT_H__
#define __PIT_H__

  // PIT I/O ports
  #define PIT_CHANNEL0          0x40
  #define PIT_CHANNEL1          0x41
  #define PIT_CHANNEL2          0x42
  #define PIT_CONTROL_WORD      0x43

  // PIT Frequency
  #define PIT_FREQUENCY         0x1234DC

  // modes for PIT_CONTROL_WORD
  #define PIT_CTR0              0x00
  #define PIT_CTR1              0x40
  #define PIT_CTR2              0x80

  #define PIT_CL                0x00
  #define PIT_LSB               0x10
  #define PIT_MSB               0x20
  #define PIT_LSBMSB            0x30

  #define PIT_MODE0             0x00
  #define PIT_MODE1             0x02
  #define PIT_MODE2             0x04
  #define PIT_MODE3             0x08
  #define PIT_MODE4             0x0A
  #define PIT_MODE5             0x0C

  #define PIT_B16               0x00
  #define PIT_BCD               0x01

  int pit_set_frequency (float count_value);

#endif //__PIT_H__
