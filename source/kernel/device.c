/******************************************************************************
 *
 *  File        : device.c
 *  Description : Device driver handling
 *
 *****************************************************************************/

#include "kernel.h"
#include "device.h"
#include "vfs.h"

// This entry holds ALL major devices
// @TODO: This should done through a queue (ll)
device_t *ll_devices;

/**
 *
 */
void device_init (void) {
  ll_devices = NULL;
}


/**
 * Adds device entry to the device list and creates file system entry
 * @TODO: Use linked list for devices as well
 */
int device_register (device_t *dev, const char *filename) {
  device_t *tmp = ll_devices;

//  kprintf ("device_register (device_t *dev, const char *%s) {\n", filename);

  // There are no devices yet, this is the first device. Special case
  if (ll_devices == NULL) {
    ll_devices = dev;
    dev->next = NULL;
  } else {
    // See if device already exists
    while (tmp) {
      if (tmp->major_num == dev->major_num && tmp->minor_num == dev->minor_num) return 0;
      tmp = (device_t *)tmp->next;
    }

    // Send end of device list
    tmp = ll_devices;
    while (tmp->next) tmp = (device_t *)tmp->next;

    // Add device to end
    tmp->next = (struct device_t *)dev;
    dev->next = NULL;
  }


  // Create device node
  vfs_node_t *node = vfs_get_node_from_path ("DEVICE:/");
  vfs_mknod (node, filename, FS_BLOCKDEVICE, dev->major_num, dev->minor_num);

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
    if (tmp->major_num == dev->major_num && tmp->minor_num == dev->minor_num) {
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
device_t *device_get_device (int major_num, int minor_num) {
  device_t *dev = ll_devices;

  while (dev) {
    // Found?
    if (dev->major_num == major_num && dev->minor_num == minor_num) return dev;
    dev = (device_t *)dev->next;
  }
  return NULL;
}

