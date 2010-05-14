/******************************************************************************
 *
 *  File        : fat12.c
 *  Description : Fat12 filesystem
 *
 *****************************************************************************/

#include "vfs.h"
#include "vfs/fat12.h"
#include "kernel.h"
#include "kmem.h"
#include "drivers/floppy.h"


fat12_bpb_t *_bpb;      // Bios Parameter Block
fat12_fat_t *_fat;      // FAT table


/**
 *
 */
Uint32 fat12_read (fs_node_t *node, Uint32 offset, Uint32 size, char *buffer) {
    return (node->read == NULL) ? NULL : node->read (node, offset, size, buffer);
}


/**
 *
 */
Uint32 fat12_write (fs_node_t *node, Uint32 offset, Uint32 size, char *buffer) {
    return (node->write == NULL) ? NULL : node->write (node, offset, size, buffer);
}


/**
 *
 */
void fat12_open (fs_node_t *node) {
    return (node->open == NULL) ? NULL : node->open (node);
}


/**
 *
 */
void fat12_close (fs_node_t *node) {
    return (node->close == NULL) ? NULL : node->close (node);
}


/**
 *
 */
struct dirent *fat12_readdir (fs_node_t *node, Uint32 index) {
    // Check if it's a directory
    if ((node->flags & 0x7) != FS_DIRECTORY) return NULL;
    return (node->readdir == NULL) ? NULL : node->readdir (node, index);
}

/**
 *
 */
fs_node_t *fat12_finddir (fs_node_t *node, char *name) {
    // Check if it's a directory
    if ((node->flags & 0x7) != FS_DIRECTORY) return NULL;
    return (node->finddir == NULL) ? NULL : node->finddir (node, name);
}







/**
 * Reads the value found on cluster entry. Should be easy, but made difficult because
 * data is stored in 12 bits instead of 16.
 */
Uint16 fat12_get_fat_entry (Uint16 cluster_entry) {
  Uint32 offset = cluster_entry + (cluster_entry / 2);
  Uint16 next_cluster = * ( (Uint16 *)(_fat + offset));

  if (cluster_entry % 2 == 0) {
    next_cluster &= 0x0fff;
  } else {
    next_cluster >>= 4;
  }
  return next_cluster;
}



/**
 *
 */
fs_node_t *fat12_init (int driveNum) {
  // Allocate root node
  fs_root = (fs_node_t *)kmalloc (sizeof (fs_node_t));
  strncpy (fs_root->name, "root", 4);   // Never referenced, so name does not matter

  fs_root->owner =  fs_root->inode = fs_root->length = 0;
  fs_root->flags = FS_DIRECTORY;

  fs_root->read = &fat12_read;
  fs_root->write = &fat12_write;
  fs_root->open = &fat12_open;
  fs_root->close = &fat12_close;
  fs_root->readdir = &fat12_readdir;
  fs_root->finddir = &fat12_finddir;

  fs_root->ptr = 0;



  // Allocate and read BPB
kprintf ("Reading BPB\n");
  _bpb = (fat12_bpb_t *)kmalloc (sizeof (fat12_bpb_t));
  fdc_read_floppy_sector (&fdc[0].drives[0], 0, (char *)_bpb);

    kprintf ("OEMName           %c%c%c%c%c%c%c%c\n", _bpb->OEMName[0],_bpb->OEMName[1],_bpb->OEMName[2],_bpb->OEMName[3],_bpb->OEMName[4],_bpb->OEMName[5],_bpb->OEMName[6],_bpb->OEMName[7]);
    kprintf ("BytesPerSector    %04x\n", _bpb->BytesPerSector);
    kprintf ("SectorsPerCluster %02x\n", _bpb->SectorsPerCluster);
    kprintf ("ReservedSectors   %04x\n", _bpb->ReservedSectors);
    kprintf ("NumberOfFats      %02x\n", _bpb->NumberOfFats);
    kprintf ("NumDirEntries     %04x\n", _bpb->NumDirEntries);
    kprintf ("NumSectors        %04x\n", _bpb->NumSectors);
    kprintf ("Media             %02x\n", _bpb->Media);
    kprintf ("SectorsPerFat     %04x\n", _bpb->SectorsPerFat);
    kprintf ("SectorsPerTrack   %04x\n", _bpb->SectorsPerTrack);
    kprintf ("HeadsPerCyl       %04x\n", _bpb->HeadsPerCyl);
    kprintf ("HiddenSectors     %08x\n", _bpb->HiddenSectors);
    kprintf ("LongSectors       %08x\n", _bpb->LongSectors);

kprintf ("Reading FAT\n");
  int i,j,k;
  _fat = (fat12_fat_t *)kmalloc (_bpb->BytesPerSector * _bpb->SectorsPerFat);
  int fatSectorLBA = _bpb->HiddenSectors + _bpb->ReservedSectors;
  for (i=0; i!=(_bpb->BytesPerSector * _bpb->SectorsPerFat / 512); i++) {
    fdc_read_floppy_sector (&fdc[0].drives[0], fatSectorLBA+i, (char *)_fat+(i*512));
  }

    for (k=0, i=0; i!=10; i++) {
        kprintf ("  Entry %04d  : ", i*10);
        for (j=0; j!=10; j++,k++) kprintf ("%04X ", fat12_get_fat_entry (k));
        kprintf ("\n");
    }
    kprintf ("\n");


  for (;;) ;



  return fs_root;
}