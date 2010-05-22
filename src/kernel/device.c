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
 */
int device_register (device_t *dev, const char *filename) {
  device_t *tmp = devices;

  // See if device already exists
  while (tmp) {
    if (tmp->majorNum == dev->majorNum && tmp->minorNum == dev->minorNum) return 0;
    tmp = (device_t *)tmp->next;
  }

  // Send end of device list
  tmp = devices;
  while (tmp->next != NULL) tmp = (device_t *)tmp->next;

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


