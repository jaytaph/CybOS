/******************************************************************************
 *
 *  File        : pci.c
 *  Description :
 *
 *****************************************************************************/
#ifndef __PCI_H__
#define __PCI_H__

  #include "ktype.h"


  #define PCI_CONFIG_ADDRESS    0xCF8
  #define PCI_CONFIG_DATA       0xCFC

  #define PCI_MAX_BUS           256
  #define PCI_MAX_SLOT          32
  #define PCI_MAX_FUNC          8

  void pci_init (void);

#endif //__PCI_H__
