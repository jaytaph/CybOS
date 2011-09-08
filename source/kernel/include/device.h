/******************************************************************************
 *
 *  File        : device.h
 *  Description : Device driver handling
 *
 *****************************************************************************/
#ifndef __DEVICE_H__
#define __DEVICE_H__

    #include "ktype.h"

    /* All major node numbers for devices. By accessing the file, the kernel
     * knows which driver is responsible for handling */
    #define DEV_MAJOR_MISC          0   // Misc devices (null, zero, rand etc)
    #define DEV_MAJOR_FDC           1   // Floppy disks
    #define DEV_MAJOR_IDE           2   // Ide controllers
    #define DEV_MAJOR_HDC           3   // Hard disks
    #define DEV_MAJOR_CONSOLES     10   // Consoles (3,0 = kconsole)    (@TODO: not used)

    typedef struct {
      Uint8  major_num;            // Major device node
      Uint8  minor_num;            // Minor device node
      void   *data;                // Some data that might accompany the device
      
      // Block device functions
      Uint32(*read)(Uint8 major, Uint8 minor, Uint32 offset, Uint32 size, char *buffer);
      Uint32(*write)(Uint8 major, Uint8 minor, Uint32 offset, Uint32 size, char *buffer);
      void (*open)(Uint8 major, Uint8 minor);
      void (*close)(Uint8 major, Uint8 minor);
      void (*seek)(Uint8 major, Uint8 minor, Uint32 offset, Uint8 direction);

      struct device_t *next;            // Pointer to next device
    } device_t;


  int device_register (device_t *dev, const char *filename);
  int device_unregister (device_t *dev);
  void device_init (void);
  device_t *device_get_device (int major_num, int minor_num);

#endif // __DEVICE_H__