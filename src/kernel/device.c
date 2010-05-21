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
int register_device (device_t *dev, const char *filename) {
  device_t *tmp = devices;

  // See if device already exists
  while (tmp) {
    if (tmp->majorNum == majorNum && tmp->minorNum == dev->minorNum) return 0;
    tmp = tmp->next;
  }

  // Send end of device list
  device_t *tmp = devices;
  while (tmp->next != NULL) tmp = tmp->next;

  // Add device to end
  tmp->next = dev;
  dev->next = NULL;

  // @TODO: add filename to /DEVICES/filename directory


  return 1;
}


/**
 * Remove device from device list. Does not reclaim memory for device!
 */
int unregister_device (device_node *dev) {
  device_t *prev, *tmp;
  prev = NULL;

  while (tmp) {
    // Found?
    if (tmp->majorNum == majorNum && tmp->minorNum == dev->minorNum) {
      // If it's not the first one, the previous item points to the next.
      if (prev != NULL) prev->next = tmp->next;
      return 1;
    }
    prev = tmp;
    tmp = tmp->next;
  }

  // Device not found
  return 0;
}


