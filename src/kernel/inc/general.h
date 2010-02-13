/******************************************************************************
 *
 *  File        : general.h
 *  Description : General functions and defines which are usefull for the
 *                kernel.
 *
 *****************************************************************************/

#ifndef __GENERAL_H__
#define __GENERAL_H__

  #include "ktype.h"

  // Break up a 32 bits word into hi and lo 16
  #define LO16(addr32) (Uint16) (addr32 & 0x0000FFFF)
  #define HI16(addr32) (Uint16)((addr32 & 0xFFFF0000) >> 16)

  // Break up a 16 bits word into hi and lo 8
  #define LO8(addr16) (Uint8) (addr16 & 0x00FF)
  #define HI8(addr16) (Uint8)((addr16 & 0xFF00) >> 8)

  // Creates a selector from a descriptor plus the flags
  #define SEL(descr,flags) (Uint32)((descr<<3)+flags)

#endif //__GENERAL_H__
