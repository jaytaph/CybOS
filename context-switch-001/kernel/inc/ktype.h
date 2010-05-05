/******************************************************************************
 *
 *  File        : ktype.h
 *  Description : Standard ctypes. Still used a little bit wildly trough the
 *                kernel. Must fix later...
 *
 *****************************************************************************/

#ifndef __KTYPE_H__
#define __KTYPE_H__

  typedef unsigned char           Uint8;
  typedef unsigned short int      Uint16;
  typedef unsigned long int       Uint32;
  typedef unsigned long long      Uint64;

  typedef signed char             Sint8;        // 8 bits signed
  typedef signed short int        Sint16;       // ...
  typedef signed long int         Sint32;
  typedef signed long long        Sint64;

#endif // __KTYPE_H__
