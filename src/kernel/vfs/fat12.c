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

typedef struct {
	Uint8  numSectors;
	Uint32 fatOffset;
	Uint8  fatSizeBytes;
	Uint8  fatSizeSectors;
	Uint8  fatEntrySizeBits;
	Uint32 numRootEntries;
	Uint32 numRootEntriesPerSector;
	Uint32 rootEntrySectors;
	Uint32 rootOffset;
	Uint32 rootSizeSectors;
	Uint32 dataOffset;
} fat12_fatinfo_t;

fat12_fatinfo_t fat12_info;

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

void fat12_convert_long_to_dos_filename (const char *longName, char *dosName) {
  int i;

  // C-terminated string with 11 spaces
  for (i=0; i!=11; i++) dosName[i] = ' ';
  dosName[11] = 0;

  // Find offset of the extension
  i=0;
  char *extension = NULL;
  while (longName[i] && longName[i] != '.') i++;
  if (longName[i] == '.') extension = (char *)(longName+i);

  // Split filename and extension
  if (extension) {
    *extension = 0; // Split string,  filename and extension are now 2 separated c-strings
    extension++;

    // Extension is the filename, this is a dot-file so there is no extension
    if (extension == longName) extension = 0;
  }

  // Copy filename (max 8 chars)
  i=0;
  while (longName[i] && i < 8) {
    dosName[0+i] = longName[i];
    i++;
  }

  // Copy extension to correct spot if any (max 3 chars)
  if (extension) {
    i=0;
    while (extension[i] && i < 3) {
      dosName[8+i] = extension[i];
      i++;
    }
  }
}


/**
 *
 */
fat12_dirent_t *fat12_search_root_directory (const char *dirName) {
  fat12_dirent_t *dirent;
  char dosName[12];
  int i,j;

  // Convert filename to FAT name
  fat12_convert_long_to_dos_filename (dirName, dosName);

  kflush ();
  kprintf ("LONG2DOS : '%s'", dosName);

  // Browse all root directory sectors
  fat12_dirent_t *direntbuf = (fat12_dirent_t *)kmalloc (_bpb->BytesPerSector);
  for (i=0; i!=fat12_info.rootSizeSectors; i++) {
    // Read directory sector
    fdc_read_floppy_sector (&fdc[0].drives[0], fat12_info.rootOffset+i, (char *)direntbuf);

    // Browse all entries in the sector
    for (dirent = direntbuf, j=0; j!=fat12_info.numRootEntriesPerSector; j++,dirent+=sizeof (fat12_dirent_t)) {
      // No more entries when first char of filename is 0
      if (dirent->Filename[0] == 0) return NULL;

      // If this is the correct file, create inode
      if (strncmp (dosName, (char *)dirent->Filename, 11) == 0) return dirent;
    } // for entries
  } // for sectors

  return NULL;
}


/**
 *
 */
fs_node_t *fat12_init (int driveNum) {
  // Allocate root node
  fs_root = (fs_node_t *)kmalloc (sizeof (fs_node_t));
  strncpy (fs_root->name, "root", 4);   // Never referenced, so name does not matter

  fs_root->owner =  fs_root->inode_nr = fs_root->length = 0;
  fs_root->flags = FS_DIRECTORY;

  fs_root->read = &fat12_read;
  fs_root->write = &fat12_write;

  fs_root->open = &fat12_open;
  fs_root->close = &fat12_close;
  fs_root->readdir = &fat12_readdir;
  fs_root->finddir = &fat12_finddir;

  fs_root->ptr = 0;


  // Allocate and read BPB
  _bpb = (fat12_bpb_t *)kmalloc (sizeof (fat12_bpb_t));
  fdc_read_floppy_sector (&fdc[0].drives[0], 0, (char *)_bpb);

/*
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
*/

    // Set global FAT information
    fat12_info.numSectors              = _bpb->NumSectors;
    fat12_info.fatOffset               = _bpb->ReservedSectors + _bpb->HiddenSectors;
    fat12_info.fatSizeBytes            = _bpb->BytesPerSector *_bpb->SectorsPerFat;
    fat12_info.fatSizeSectors          = _bpb->SectorsPerFat;
    fat12_info.fatEntrySizeBits        = 8;
    fat12_info.numRootEntries          = _bpb->NumDirEntries;
    fat12_info.numRootEntriesPerSector = _bpb->BytesPerSector / 32;
    fat12_info.rootOffset              = (_bpb->NumberOfFats * _bpb->SectorsPerFat) + 1;
    fat12_info.rootSizeSectors         = (_bpb->NumDirEntries * 32 ) / _bpb->BytesPerSector;
    fat12_info.dataOffset              = fat12_info.rootOffset + (_bpb->NumberOfFats * _bpb->SectorsPerFat) + 1 - 2;

kprintf ("FAT12INFO\n");
kprintf ("numSectors        %04X\n", fat12_info.numSectors);
kprintf ("fatOffset         %04X\n", fat12_info.fatOffset);
kprintf ("fatSizeBytes      %04X\n", fat12_info.fatSizeBytes);
kprintf ("fatSizeSectors    %04X\n", fat12_info.fatSizeSectors);
kprintf ("fatEntrySizeBits  %04X\n", fat12_info.fatEntrySizeBits);
kprintf ("numRootEntries    %04X\n", fat12_info.numRootEntries);
kprintf ("numRootEntriesPS  %04X\n", fat12_info.numRootEntriesPerSector);
kprintf ("rootOffset        %04X\n", fat12_info.rootOffset);
kprintf ("rootSizeSectors   %04X\n", fat12_info.rootSizeSectors);
kprintf ("dataOffset        %04X\n", fat12_info.dataOffset);
kprintf ("----------------\n");


  int i;
  _fat = (fat12_fat_t *)kmalloc (fat12_info.fatSizeBytes);
  for (i=0; i!=fat12_info.fatSizeSectors; i++) {
    fdc_read_floppy_sector (&fdc[0].drives[0], fat12_info.fatOffset+i, (char *)_fat+(i*512));
  }
/*
  for (k=0, i=0; i!=10; i++) {
    kprintf ("  Entry %04d  : ", i*10);
    for (j=0; j!=10; j++,k++) kprintf ("%04X ", fat12_get_fat_entry (k));
    kprintf ("\n");
  }
  kprintf ("\n");
*/

/*
  fat12_dirent_t *direntbuf = (fat12_dirent_t *)kmalloc (_bpb->BytesPerSector);
  fat12_dirent_t *dirent;
  for (i=0; i!=fat12_info.rootSizeSectors; i++) {
    fdc_read_floppy_sector (&fdc[0].drives[0], fat12_info.rootOffset+i, (char *)direntbuf);
    for (dirent = direntbuf, j=0; j!=fat12_info.numRootEntriesPerSector; j++,dirent++) {
      kprintf ("-------------\n");
      kprintf ("Filename     : %c%c%c%c%c%c%c%c\n", dirent->Filename[0], dirent->Filename[1], dirent->Filename[2], dirent->Filename[3], dirent->Filename[4], dirent->Filename[5], dirent->Filename[6], dirent->Filename[7]);
      kprintf ("Ext          : %c%c%c\n", dirent->Ext[0], dirent->Ext[1], dirent->Ext[2]);
      kprintf ("Attrib       : %02X\n", dirent->Attrib);
      kprintf ("FirstCluster : %04X\n", dirent->FirstCluster);
      kprintf ("FileSize     : %02X\n", dirent->FileSize);
    }
  }
*/

  kprintf ("FAT12_SEARCH_ROOT_DIR\n");

  fat12_dirent_t *dirent;
  dirent = fat12_search_root_directory ("SYSTEM");
  kprintf ("%08X\n", dirent);
  dirent = fat12_search_root_directory ("KERNEL.BIN");
  kprintf ("%08X\n", dirent);
  dirent = fat12_search_root_directory ("A.OUT");
  kprintf ("%08X\n", dirent);
  dirent = fat12_search_root_directory ("1234567890AB.123456");
  kprintf ("%08X\n", dirent);

  for (;;) ;

  return fs_root;
}





/**
 *
 */
Uint16 fat12_next_cluster (Uint16 cluster) {
  Uint32 offset = cluster + (cluster / 2);
  Uint16 next_cluster = *( (Uint16 *)(_fat + offset));

  if ((cluster & 1) == 0) {
    next_cluster &= 0x0FFF;
  } else {
    next_cluster >>= 4;
  }

  return next_cluster;
}

/**
 *
 */
void fat12_read_file (fat12_file_t *file, char *buffer, Uint32 byte_count) {
  if (! file) return;

  // LBA sector
  Uint32 lba_sector = fat12_info.dataOffset + file->currentCluster;

  char *bufptr = buffer;

  // @TODO   NEED MORE WORK.. ONLY READ BYTE_COUNT buffers, not more...
  int i;
  for (i=0; i!=_bpb->SectorsPerCluster; i++) {
    fdc_read_floppy_sector (&fdc[0].drives[0], lba_sector, bufptr);
    bufptr += _bpb->BytesPerSector;
  }

  file->currentCluster = fat12_next_cluster (file->currentCluster);
  if (file->currentCluster < 2 || file->currentCluster >= 0xFF8) {
    file->eof = 1;
    return;
  }

/*
  // Pointer to  buffer
  char *bufptr = buffer;

  Uint32 bytes_left = byte_count;

  // Size of a cluster
  Uint32 cluster_size = _bpb->SectorsPerCluster * _bpb->BytesPerSector;

  // Buffer to hold 1 cluster
  char *cluster = (char *)kmalloc (cluster_size);

  // Current cluster number we are reading
  Uint32 current_cluster = file->FirstCluster;

  do {
    // Read cluster X
    int i;
    for (i=0; i!=_bpb->SectorsPerCluster; i++) {   // @TODO Incorrect when bytespersectors != sector size
      fdc_read_floppy_sector (&fdc[0].drives[0], fat12_info.dataOffset+i, (char *)cluster+(i*_bpb->BytesPerSector));
    }

    if (bytes_left != -1 && bytes_left < cluster_size) {
      // If filesize is available, and we have less than 1 cluster left to read, read remaining cluster size
      i = bytes_left;
    } else {
      // Read complete cluster
      i = cluster_size;
    }

    // Copy bytes into buffer
    memcpy (bufptr, cluster, i);

    // Increase buffer pointer
    bufptr+=i;

    // Decrease bytesLeft
    if (bytes_left > -1) bytes_left -= i;

    // Get next cluster from FAT table
    current_cluster = fat12_next_cluster (current_cluster);
  } while (current_cluster > 0x002 && current_cluster < 0xFF8);

  // Free cluster memory
  kfree ((Uint32)cluster);
*/
}


/**
 *
 */
fat12_file_t *fat12_find_subdir (fat12_file_t *file, const char *dirName) {
  char dosName[12];
  int i;

  // Convert filename to FAT name
  fat12_convert_long_to_dos_filename (dirName, dosName);

  // Do as long as we have file entries (always padded on sector which is always divved by 512)
  while (! file->eof) {
    char buf[512];
    fat12_read_file (file, buf, 512);

    // Browse all entries
    fat12_dirent_t *dirptr = (fat12_dirent_t *)&buf;
    for (i=0; i!=512 / sizeof (fat12_dirent_t); i++) {
      // Filename is 0, no more entries (only padding and this is the last sector
      if (dirptr->Filename[0] == 0) return NULL;

      // check if this is the correct directory entry
      if (strncmp (dirptr->Filename, dirName, 11) == 0) {
        // Create tmpfile
        fat12_file_t *tmpfile = (fat12_file_t *)kmalloc (sizeof(fat12_file_t));

        // Set info
        strcpy (tmpfile->name, dirName);
        tmpfile->eof = 0;
        tmpfile->length = dirptr->FileSize;
        tmpfile->offset = 0;
        tmpfile->flags = (dirptr->Attrib == 0x10) ? FS_DIRECTORY : FS_FILE;
        tmpfile->currentCluster = dirptr->FirstCluster;
        tmpfile->currentOffset = 0;
        tmpfile->currentDrive = _currentDrive;

        return tmpfile;
      }
      // Try next directory entry
      dirptr++;
    }
  }

  // Nothing found
  return NULL;
}