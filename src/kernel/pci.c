/******************************************************************************
 *
 *  File        : pci.c
 *  Description :
 *
 *
 *****************************************************************************/
#include "kernel.h"
#include "kmem.h"
#include "pci.h"


/**
 *
 */
Uint32 pci_readword (Uint16 bus, Uint16 slot, Uint16 func, Uint16 offset) {
  Uint32 address = 0x80000000; // Bit 31 set
  address |= (Uint32)bus << 16;
  address |= (Uint32)slot << 11;
  address |= (Uint32)func << 8;
  address |= (Uint32)offset & 0xfc;

  outl (PCI_CONFIG_ADDRESS, address);
  Uint32 ret = inl (PCI_CONFIG_DATA);

  ret = ret >> ( (offset & 2) * 8) & 0xffff;
  return ret;
}

/**
 *
 */
void pci_init (void) {
  int bus,slot,func;

  for (bus=0; bus != PCI_MAX_BUS; bus++) {
    for (slot=0; slot != PCI_MAX_SLOT; slot++) {
      Uint16 vendor = pci_readword (bus, slot, 0, 0);
      if (vendor == 0xFFFF) continue;  // Nothing on this slot

      for (func=0; func != PCI_MAX_FUNC; func++) {

        // Read additional info
        Uint16 device = pci_readword (bus, slot, func, 2);
        if (device == 0xFFFF) continue;  // Nothing on this slot

        Uint16 rev = pci_readword (bus, slot, func, 8);
        rev = LO8(rev);

        Uint16 tmp = pci_readword (bus, slot, func, 10);
        Uint16 class_code = HI8(tmp);
        Uint16 subclass_code = LO8(tmp);

        Uint16 header = pci_readword (bus, slot, func, 14);
        header = LO8(header);

//        kprintf ("PCI [%02X.%02X.%02X] [%02X:%02X] Device %04X:%04X (rev %d)\n", bus, slot, func, class_code, subclass_code, vendor, device, rev);

        // Don't check next function. This device is single function
        if ( func == 0 && (header & 0x80) != 0x80) break;
      } // func
    } // slot
  } // bus

}