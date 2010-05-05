/******************************************************************************
 *
 *  File        : version.h
 *  Description : standard version defines for the kernel. Manually editted
 *                on compilation time.
 *
 *****************************************************************************/

#ifndef __VERSION_H__
#define __VERSION_H__

  #define KERNEL_VERSION_MAJOR   "0"
  #define KERNEL_VERSION_MINOR   "01"

  #define KERNEL_COMPILER       "jthijssen@noxlogic.nl"

  #define KERNEL_COMPILE_TIME   __TIME__
  #define KERNEL_COMPILE_DATE   __DATE__

#endif // __VERSION_H__
