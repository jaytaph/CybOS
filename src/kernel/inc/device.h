/******************************************************************************
 *
 *  File        : device.h
 *  Description : Device driver handling
 *
 *****************************************************************************/
#ifndef __DEVICE_H__
#define __DEVICE_H__

    #include "ktype.h"

    typedef struct {
      Uint8  majorNum;            // Major device node
      Uint8  minorNum;            // Minor device node

      // Block device functions
      Uint32(*read)(Uint8 major, Uint8 minor, Uint32 offset, Uint32 size, char *buffer);
      Uint32(*write)(Uint8 major, Uint8 minor, Uint32 offset, Uint32 size, char *buffer);
      void (*open)(Uint8 major, Uint8 minor);
      void (*close)(Uint8 major, Uint8 minor);
      void (*seek)(Uint8 major, Uint8 minor, Uint32 offset, Uint8 direction);

      struct device_t *next;            // Pointer to next device
    } device_t;


  int device_register (device_t *dev, const char *filename);
  int device_unregister (device_node *dev);
  void device_init (void);

#endif // __DEVICE_H__