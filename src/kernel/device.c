/******************************************************************************
 *
 *  File        : device.c
 *  Description : Device driver handling
 *
 *****************************************************************************/

#include "kernel.h"
#include "device.h"

// This entry holds ALL major devices
device_t *devices;

/**
 *
 */
void device_init (void) {
  devices = NULL;
}


/**
 * Adds device entry to the device list and creates file system entry
 * @TODO: Use linked list for devices as well
 */
int device_register (device_t *dev, const char *filename) {
  device_t *tmp = devices;

  // There are no devices yet, this is the first device. Special case
  if (devices == NULL) {
    devices = dev;
    dev->next = NULL;
    return 1;
  }

  // See if device already exists
  while (tmp) {
    if (tmp->majorNum == dev->majorNum && tmp->minorNum == dev->minorNum) return 0;
    tmp = (device_t *)tmp->next;
  }

  // Send end of device list
  tmp = devices;
  while (tmp->next) tmp = (device_t *)tmp->next;

  // Add device to end
  tmp->next = (struct device_t *)dev;
  dev->next = NULL;

  // @TODO: add filename to /DEVICES/filename directory ??

  return 1;
}


/**
 * Remove device from device list. Does not reclaim memory for device!
 */
int device_unregister (device_t *dev) {
  device_t *prev, *tmp;
  prev = NULL;

  while (tmp) {
    // Found?
    if (tmp->majorNum == dev->majorNum && tmp->minorNum == dev->minorNum) {
      // If it's not the first one, the previous item points to the next.
      if (prev != NULL) prev->next = tmp->next;
      return 1;
    }
    prev = tmp;
    tmp = (device_t *)tmp->next;
  }

  // Device not found
  return 0;
}

/**
 *
 */
device_t *device_get_device (int majorNum, int minorNum) {
  device_t *dev = devices;

  while (dev) {
    // Found?
    if (dev->majorNum == majorNum && dev->minorNum == minorNum) return dev;
    dev = (device_t *)dev->next;
  }
  return NULL;
}

