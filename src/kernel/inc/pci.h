/******************************************************************************
 *
 *  File        : pci.c
 *  Description :
 *
 *****************************************************************************/
#ifndef __PCI_H__
#define __PCI_H__

  #include "ktype.h"

  #define PCI_MAX_DEVICES     255

  typedef struct {
    Uint16  enabled;
    Uint16  index;      // Needed because we want to lookup index when only the device is known occasionally
    Uint16  class;
    Uint16  subclass;
    Uint16  vendor_id;
    Uint16  device_id;
    Uint16  bus;        // Where on the PCI is this device
    Uint16  slot;
    Uint16  func;
    char    config_space[256];
  } pci_device_t;


  #define PCI_CONFIG_ADDRESS    0xCF8
  #define PCI_CONFIG_DATA       0xCFC

  #define PCI_MAX_BUS           256
  #define PCI_MAX_SLOT          32
  #define PCI_MAX_FUNC          8

  void pci_init (void);
  pci_device_t *pci_find_next_class (pci_device_t *idx, int class, int subclass);
  pci_device_t *pci_find_next_position (pci_device_t *dev, int bus, int slot, int func);

  Uint8 pci_config_get_byte (pci_device_t *dev, int offset);
  Uint16 pci_config_get_word (pci_device_t *dev, int offset);
  Uint32 pci_config_get_dword (pci_device_t *dev, int offset);

#endif //__PCI_H__
