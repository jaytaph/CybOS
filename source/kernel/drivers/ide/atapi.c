/******************************************************************************
 *
 *  File        : atapi.c
 *  Description : ATAPI interface
 *
 *
 *****************************************************************************/

#include "drivers/ide.h"
#include "drivers/ide_atapi.h"
#include "kernel.h"
#include "kmem.h"
#include "pci.h"


Uint32 ide_atapi_access(char direction, ide_drive_t *drive, Uint32 lba_sector, Uint32 sector_count, char *buf) {
  // @TODO: create atapi
  kpanic ("IDE_ATAPI_ACCESS not supported");
  return 0;
}