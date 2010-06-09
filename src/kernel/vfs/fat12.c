/******************************************************************************
 *
 *  File        : fat12.c
 *  Description : Fat12 filesystem
 *
 *  TODO: Wrong things happend when clustersize != sectorsize != 512!!!
 *
 *****************************************************************************/

#include "kernel.h"
#include "kmem.h"
#include "vfs.h"
#include "vfs/fat12.h"
#include "drivers/floppy.h"

// File operations
static struct vfs_fileops fat12_fileops = {
    .read = fat12_read, .write = fat12_write,
    .open = fat12_open, .close = fat12_close,
    .readdir = fat12_readdir, .finddir = fat12_finddir
};

// Mount operations
static struct vfs_mountops fat12_mountops = {
    .mount = fat12_mount, .umount = fat12_umount
};

// Global structure
static vfs_info_t fat12_vfs_info = { .tag = "fat12",
                                     .name = "FAT12 File System",
                                     .mountops = &fat12_mountops
                                   };

// Root supernode
static vfs_node_t fat12_supernode = {
  .inode_nr = 0,
  .name = "/",
  .owner = 0,
  .length = 0,
  .flags = FS_DIRECTORY,
  .majorNum = 0,
  .minorNum = 0,
  .fileops = &fat12_fileops,
};

vfs_dirent_t dirent;   // Entry that gets returned by fat12_readdir
vfs_node_t filenode;   // Entry that gets returned by fat12_finddir

// Forward defines
void fat12_convert_dos_to_c_filename (char *longName, const char *dosName);
void fat12_convert_c_to_dos_filename (const char *longName, char *dosName);
Uint16 fat12_get_next_cluster (fat12_fat_t *fat, Uint16 cluster);



/**
 * Called when a device that holds a FAT12 image gets mounted onto a mount_point
 */
vfs_node_t *fat12_mount (struct vfs_mount *mount, device_t *dev, const char *path) {
//  kprintf ("fat12_mount\n");

  // Allocate fat12_info for this mount
  mount->fs_data = kmalloc (sizeof (fat12_fatinfo_t));
  fat12_fatinfo_t *fat12_info = mount->fs_data; // Alias for easier usage
  if (!fat12_info) goto cleanup;
  memset (fat12_info, 0, sizeof (fat12_fatinfo_t));

  // Allocate and read BPB
  fat12_info->bpb = (fat12_bpb_t *)kmalloc (sizeof (fat12_bpb_t));
  if (!fat12_info->bpb) goto cleanup;
  memset (fat12_info->bpb, 0, sizeof (fat12_bpb_t));

  // Read boot sector from disk
  int rb = mount->dev->read (mount->dev->majorNum, mount->dev->minorNum, 0, 512, (char *)fat12_info->bpb);
  if (rb != 512) {
    kprintf ("Cannot read enough bytes from boot sector (%d read, %d needed)\n", rb, fat12_info->bpb->BytesPerSector);
    goto cleanup;
  }

/*
  kprintf ("OEMName           %c%c%c%c%c%c%c%c\n", fat12_info->bpb->OEMName[0],fat12_info->bpb->OEMName[1],fat12_info->bpb->OEMName[2],fat12_info->bpb->OEMName[3],fat12_info->bpb->OEMName[4],fat12_info->bpb->OEMName[5],fat12_info->bpb->OEMName[6],fat12_info->bpb->OEMName[7]);
  kprintf ("BytesPerSector    %04x\n", fat12_info->bpb->BytesPerSector);
  kprintf ("SectorsPerCluster %02x\n", fat12_info->bpb->SectorsPerCluster);
  kprintf ("ReservedSectors   %04x\n", fat12_info->bpb->ReservedSectors);
  kprintf ("NumberOfFats      %02x\n", fat12_info->bpb->NumberOfFats);
  kprintf ("NumDirEntries     %04x\n", fat12_info->bpb->NumDirEntries);
  kprintf ("NumSectors        %04x\n", fat12_info->bpb->NumSectors);
  kprintf ("Media             %02x\n", fat12_info->bpb->Media);
  kprintf ("SectorsPerFat     %04x\n", fat12_info->bpb->SectorsPerFat);
  kprintf ("SectorsPerTrack   %04x\n", fat12_info->bpb->SectorsPerTrack);
  kprintf ("HeadsPerCyl       %04x\n", fat12_info->bpb->HeadsPerCyl);
  kprintf ("HiddenSectors     %08x\n", fat12_info->bpb->HiddenSectors);
  kprintf ("LongSectors       %08x\n", fat12_info->bpb->LongSectors);
*/

  // Set global FAT information as read and calculated from the bpb
  fat12_info->numSectors              = fat12_info->bpb->NumSectors;
  fat12_info->fatOffset               = fat12_info->bpb->ReservedSectors + fat12_info->bpb->HiddenSectors;
  fat12_info->fatSizeBytes            = fat12_info->bpb->BytesPerSector * fat12_info->bpb->SectorsPerFat;
  fat12_info->fatSizeSectors          = fat12_info->bpb->SectorsPerFat;
  fat12_info->fatEntrySizeBits        = 8;
  fat12_info->numRootEntries          = fat12_info->bpb->NumDirEntries;
  fat12_info->numRootEntriesPerSector = fat12_info->bpb->BytesPerSector / 32;
  fat12_info->rootOffset              = (fat12_info->bpb->NumberOfFats * fat12_info->bpb->SectorsPerFat) + 1;
  fat12_info->rootSizeSectors         = (fat12_info->bpb->NumDirEntries * 32 ) / fat12_info->bpb->BytesPerSector;
  fat12_info->dataOffset              = fat12_info->rootOffset + fat12_info->rootSizeSectors - 2;

/*
  kprintf ("FAT12INFO\n");
  kprintf ("numSectors        %04X\n", fat12_info->numSectors);
  kprintf ("fatOffset         %04X\n", fat12_info->fatOffset);
  kprintf ("fatSizeBytes      %04X\n", fat12_info->fatSizeBytes);
  kprintf ("fatSizeSectors    %04X\n", fat12_info->fatSizeSectors);
  kprintf ("fatEntrySizeBits  %04X\n", fat12_info->fatEntrySizeBits);
  kprintf ("numRootEntries    %04X\n", fat12_info->numRootEntries);
  kprintf ("numRootEntriesPS  %04X\n", fat12_info->numRootEntriesPerSector);
  kprintf ("rootOffset        %04X\n", fat12_info->rootOffset);
  kprintf ("rootSizeSectors   %04X\n", fat12_info->rootSizeSectors);
  kprintf ("dataOffset        %04X\n", fat12_info->dataOffset);
  kprintf ("----------------\n");
*/

  // Read (primary) FAT table
  int i;
  fat12_info->fat = (fat12_fat_t *)kmalloc (fat12_info->fatSizeBytes);
  if (!fat12_info->fat) goto cleanup;

  memset (fat12_info->fat, 0, fat12_info->fatSizeBytes);

  for (i=0; i!=fat12_info->fatSizeSectors; i++) {
    Uint32 offset = (fat12_info->fatOffset+i) * fat12_info->bpb->BytesPerSector;
    int rb = mount->dev->read (mount->dev->majorNum, mount->dev->minorNum, offset, fat12_info->bpb->BytesPerSector, (char *)fat12_info->fat+(i*512));
    if (rb != fat12_info->bpb->BytesPerSector) {
      kprintf ("Cannot read fat sector %d\n", i);
      goto cleanup;
    }
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
  for (i=0; i!=fat12_info->rootSizeSectors; i++) {
    Uint32 offset = (fat12_info->rootOffset+i) * fat12_info->bpb->BytesPerSector;
//@TODO    node->block->read (node->block->major, node->block->minor, offset, fat12_info->bpb->BytesPerSector, direntbuf);

    for (dirent = direntbuf, j=0; j!=fat12_info->numRootEntriesPerSector; j++,dirent++) {
      kprintf ("-------------\n");
      kprintf ("Filename     : %c%c%c%c%c%c%c%c\n", dirent->Filename[0], dirent->Filename[1], dirent->Filename[2], dirent->Filename[3], dirent->Filename[4], dirent->Filename[5], dirent->Filename[6], dirent->Filename[7]);
      kprintf ("Ext          : %c%c%c\n", dirent->Ext[0], dirent->Ext[1], dirent->Ext[2]);
      kprintf ("Attrib       : %02X\n", dirent->Attrib);
      kprintf ("FirstCluster : %04X\n", dirent->FirstCluster);
      kprintf ("FileSize     : %02X\n", dirent->FileSize);
    }
  }
*/

//  kprintf ("Returning FAT12's supernode\n");
  return &fat12_supernode;


  // Jumped to when error during mount
cleanup:
  if (fat12_info && fat12_info->fat) kfree ((Uint32)fat12_info->fat);
  if (fat12_info && fat12_info->bpb) kfree ((Uint32)fat12_info->bpb);
  if (fat12_info) kfree ((Uint32)fat12_info);
  return NULL;
}

/**
 * Called when a mount_point gets unmounted
 */
void fat12_umount (struct vfs_mount *mount) {
//  kprintf ("Unmounting fat12_info structure");
  // Free our fs_data
  fat12_fatinfo_t *fat12_info = (fat12_fatinfo_t *)mount->fs_data;

  kfree ((Uint32)fat12_info->fat);
  kfree ((Uint32)fat12_info->bpb);
  kfree ((Uint32)fat12_info);
}



/**
 *
 */
Uint32 fat12_read (struct vfs_mount *mount, vfs_node_t *node, Uint32 offset, Uint32 size, char *buffer) {
  return 0;//    return node->block->read (node->block->major, node->block->minor, offset, size, buffer);
}

/**
 *
 */
Uint32 fat12_write (struct vfs_mount *mount, vfs_node_t *node, Uint32 offset, Uint32 size, char *buffer) {
  return 0;
//  return node->block->write (node->block->major, node->block->minor, offset, size, buffer);
}

/**
 *
 */
void fat12_open (struct vfs_mount *mount, vfs_node_t *node) {
//  node->block->open (node->block->major, node->block->minor);
}

/**
 *
 */
void fat12_close (struct vfs_mount *mount, vfs_node_t *node) {
//  node->block->close (node->block->major, node->block->minor);
}


/**
 *
 */
vfs_dirent_t *fat12_readdir (struct vfs_mount *mount, vfs_node_t *node, Uint32 index) {
  int i;

  fat12_fatinfo_t *fat12_info = mount->fs_data; // Alias for easier usage

//  kprintf ("fat12_readdir\n");

  // Check if it's a directory
  if ((node->flags & 0x7) != FS_DIRECTORY) return NULL;

  // Every sector holds this many directories
  int dirsPerSector = fat12_info->bpb->BytesPerSector / 32;
  int sectorNeeded = index / dirsPerSector;   // Sector we need (@TODO: problem when sector != cluster)
  int indexNeeded = index % dirsPerSector;    // N'th entry in this sector needed

  // Create entry that holds 1 sector
  fat12_dirent_t *direntbuf = (fat12_dirent_t *)kmalloc (fat12_info->bpb->BytesPerSector);
//  kprintf ("INODE_NR: %d %d\n", node->inode_nr, index);

  // Do we need to read the root directory?
  if (node->inode_nr == 0) {
//    kprintf ("Root read\n");
    Uint32 offset = (fat12_info->rootOffset+sectorNeeded) * fat12_info->bpb->BytesPerSector;
//    kprintf ("Offset: %08X\n", offset);
    mount->dev->read (mount->dev->majorNum, mount->dev->minorNum, offset, fat12_info->bpb->BytesPerSector, (char *)direntbuf);
  } else {
//    kprintf ("Cluster read read\n");
    // First start cluster (which is the INODE number :P), and seek N'th cluster
    Uint16 cluster = node->inode_nr;
    for (i=0; i!=sectorNeeded; i++) {
      cluster = fat12_get_next_cluster (fat12_info->fat, cluster);
    }

    // Read this cluster
    Uint32 offset = (fat12_info->dataOffset+cluster) * fat12_info->bpb->BytesPerSector;
//    kprintf ("Offset: %08X\n", offset);
    mount->dev->read (mount->dev->majorNum, mount->dev->minorNum, offset, fat12_info->bpb->BytesPerSector, (char *)direntbuf);
  }

  // Seek correcy entry
  fat12_dirent_t *direntbufptr = direntbuf;
  for (i=0; i!=indexNeeded; i++) {
//    kprintf ("i: %d\n", i);
    direntbufptr++;
    //direntbufptr += sizeof (fat12_dirent_t);
//    kprintf ("DBP: %08X\n", direntbufptr);
  }

//  kprintf ("%c %c %c %c\n", direntbufptr->Filename[0], direntbufptr->Filename[1], direntbufptr->Filename[2], direntbufptr->Filename[3]);

  // No more entries when first char of filename is 0
  if (direntbufptr->Filename[0] == 0) {
    kfree ((Uint32)direntbuf);
    return NULL;
  }

  // Create returning value
  fat12_convert_dos_to_c_filename (dirent.name, (char *)direntbufptr->Filename);
  dirent.inode_nr = direntbufptr->FirstCluster;

//kprintf ("dirent.name: '%s'\n", dirent.name);
  kfree ((Uint32)direntbuf);
  return &dirent;
}


/**
 * Returns:
 *   0 = end of buffer, need more data
 *   1 = found item. filenode is filled
 *   2 = no more entries
 */
int fat12_parse_directory_sector (struct vfs_mount *mount, fat12_dirent_t *direntbuf, vfs_node_t *node, const char *dosName) {
  fat12_fatinfo_t *fat12_info = mount->fs_data; // Alias for easier usage
//  kprintf ("fat12_parse_directory_sector\n");
  fat12_dirent_t *direntbufptr = direntbuf;
  int j;

  // Browse all entries in the sector
  for (j=0; j!=fat12_info->numRootEntriesPerSector; j++, direntbufptr++) {
    // No more entries when first char of filename is 0
    if (direntbufptr->Filename[0] == 0) return 2;

//    kprintf ("Equal: '%s' and '%s' \n", dosName, direntbufptr->Filename);

    // Is this the correct file?
    if (strncmp (dosName, (char *)direntbufptr->Filename, 11) == 0) {
      // Copy parent data (so we don't have to set functions etc)
      memcpy (&filenode, node, sizeof (vfs_node_t));

      // Create returning value
      filenode.inode_nr = direntbufptr->FirstCluster;
      fat12_convert_dos_to_c_filename (filenode.name, (char *)direntbufptr->Filename);
      filenode.owner = 0;
      filenode.length = direntbufptr->FileSize;
      filenode.flags = (direntbufptr->Attrib == 0x10) ? FS_DIRECTORY : FS_FILE;

      return 1;
    }
  } // for entries

  // All entries parsed, more entries probably available and not yet found
  return 0;
}


/**
 *
 */
vfs_node_t *fat12_finddir (struct vfs_mount *mount, vfs_node_t *node, const char *name) {
  fat12_fatinfo_t *fat12_info = mount->fs_data; // Alias for easier usage
  char dosName[12];
  int i;

  // Check if it's a directory
  if ((node->flags & 0x7) != FS_DIRECTORY) return NULL;

  // Convert filename to FAT name
  fat12_convert_c_to_dos_filename (name, dosName);

  // Allocate buffer memory for directory entry
  fat12_dirent_t *direntbuf = (fat12_dirent_t *)kmalloc (fat12_info->bpb->BytesPerSector);

  // Do we need to read the root directory?
  if (node->inode_nr == 0) {
    // Read all root directory sectors until we reach the correct one or end of sectors

    for (i=0; i!=fat12_info->rootSizeSectors; i++) {
      // Read directory sector
      Uint32 offset = (fat12_info->rootOffset+i) * fat12_info->bpb->BytesPerSector;
      mount->dev->read (mount->dev->majorNum, mount->dev->minorNum, offset, fat12_info->bpb->BytesPerSector, (char *)direntbuf);

      int ret = fat12_parse_directory_sector (mount, direntbuf, node, dosName);
      switch (ret) {
        case 0 :
                  // More directories needed, read next sector
                  break;
        case 1 :
                  // Directory found
                  kfree ((Uint32)direntbuf);
                  return &filenode;
        case 2 :
                  // No directory found and end of buffer
                  kfree ((Uint32)direntbuf);
                  return NULL;
      }
    } // for sectors

  } else {
    // Read 'normal' subdirectory

    Uint16 cluster = node->inode_nr;
//    kprintf ("Cluster: %04x\n", cluster);

    // Do as long as we have file entries (always padded on sector which is always divved by 512)
    do {
      // read 1 sector at a time
      Uint32 offset = (fat12_info->dataOffset+cluster) * fat12_info->bpb->BytesPerSector;
      mount->dev->read (mount->dev->majorNum, mount->dev->minorNum, offset, fat12_info->bpb->BytesPerSector, (char *)direntbuf);

      int ret = fat12_parse_directory_sector (mount, direntbuf, node, dosName);
      switch (ret) {
        case 0 :
                  // More directories needed, read next sector
                  break;
        case 1 :
                  // Directory found
                  kfree ((Uint32)direntbuf);
                  return &filenode;
        case 2 :
                  // No directory found and end of buffer
                  kfree ((Uint32)direntbuf);
                  return NULL;
      }

      // Fetch next cluster from file
      cluster = fat12_get_next_cluster (fat12_info->fat, cluster);
      kprintf ("New Cluster: %04x\n", cluster);
      // Repeat until we hit end of cluster list
    } while (cluster > 0x002 && cluster <= 0xFF7);
  }

  kfree ((Uint32)direntbuf);
  return NULL;
}



/**
 * Converts dosname '123456  .123' to longname (123456.123)
 */
void fat12_convert_dos_to_c_filename (char *longName, const char *dosName) {
  int i = 0;
  int j = 0;

  // Add chars from filename
  while (i < 8 && dosName[i] != ' ') longName[j++] = dosName[i++];

  // Add extension if any is available
  if (dosName[8] != ' ') {
    longName[j++] = '.';

    i=8;
    while (i < 11 && dosName[i] != ' ') longName[j++] = dosName[i++];
  }

  // Add termination string
  longName[j] = 0;
}


/**
 * Convert a long filename to a FAT-compatible name (8.3 chars)
 * Does:
 *  - get the first 8 chars (max) from filename
 *  - get the first 3 chars (max) from extension (if any)
 *  - support dot-files
 *  - return \0-terminated string and space-padding
 * Does not:
 *  - uppercase files
 *  - change long filenames into '~n' format
 */
void fat12_convert_c_to_dos_filename (const char *longName, char *dosName) {
  int i;

  // C-terminated string with 11 spaces
  for (i=0; i!=11; i++) dosName[i] = ' ';
  dosName[11] = 0;

  if (strcmp (longName, ".") == 0) {
    dosName[0] = '.';
    return;
  }

  if (strcmp (longName, "..") == 0) {
    dosName[0] = '.';
    dosName[1] = '.';
    return;
  }

  // Find offset of the extension
  i=0;
  char *extension = NULL;
  while (longName[i] && longName[i] != '.') i++;
  if (longName[i] == '.') extension = (char *)(longName+i);

  // Split filename and extension
  if (extension) {
    *extension = 0; // Split string,  filename and extension are now 2 separated c-strings
    extension++;

    /* The extension starts from the beginning. This means this file is a dot-file so there
     * is no extension, only a filename */
    if (extension == longName+1) extension = 0;
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
 * Initialises the FAT12 on current drive
 */
void fat12_init (void) {
  // Register filesystem to the VFS
  vfs_register_filesystem (&fat12_vfs_info);
}


/**
 * Finds the next cluster inside the FAT table
 */
Uint16 fat12_get_next_cluster (fat12_fat_t *fat, Uint16 cluster) {
  Uint32 offset = cluster + (cluster / 2);
  Uint16 next_cluster = *( (Uint16 *)(fat + offset));

  if ((cluster & 1) == 0) {
    next_cluster &= 0x0FFF;
  } else {
    next_cluster >>= 4;
  }

  return next_cluster;
}


/**
 * Read (partial) file data. Halts on last sector or end of byte_count
 *
 * Note: when hitting end of file, it will read the whole remaining sector (but never
 * more than byte_count).
 *
 * File A is 600 bytes.
 *   (file, buffer, 600) reads 600 bytes
 *   (file, buffer, 800) reads 800 bytes (200 padding)
 *   (file, buffer, 2000) reads 1024 bytes (424 padding, no more since no more clusters
 *                        for that file.
 */
void obs_fat12_read_file_data (struct vfs_mount *mount, fat12_file_t *file, char *buffer, Uint32 byte_count) {
  fat12_fatinfo_t *fat12_info = mount->fs_data; // Alias for easier usage

  // No file, no read
  if (! file) return;

  char *bufptr = buffer;

  // Number of bytes
  Uint32 bytes_left = byte_count;

  // Buffer to hold 1 cluster
  char *cluster = (char *)kmalloc (fat12_info->bpb->BytesPerSector * fat12_info->bpb->SectorsPerCluster);

  do {
    // Read current cluster  @TODO: error when cluster size != sector size!
    Uint32 offset = file->currentCluster * fat12_info->bpb->BytesPerSector;
    mount->dev->read (mount->dev->majorNum, mount->dev->minorNum, offset, fat12_info->bpb->BytesPerSector, cluster);

    // Do we have some bytes left in this cluster? Read them all by default
    int count = 512-file->currentClusterOffset;

    // Are we reading too much, read less
    if (count < bytes_left) count = bytes_left;

    // Transfer what we need to read to buffer
    memcpy (bufptr, cluster, count);
    bufptr += count; // Set next position

    // Add count to cluster offset
    file->currentClusterOffset += count;

    // Offset larger than cluster size, get next cluster
    if (file->currentClusterOffset >= 512) {
      file->currentCluster = fat12_get_next_cluster (fat12_info->fat, file->currentCluster);

      // No more clusters, we have reached end of file
      if (file->currentCluster < 2 || file->currentCluster >= 0xFF8) file->eof = 1;
    }

    // Wrap cluster offset
    file->currentClusterOffset %= 512;

    // Repeat until we have reached end of file (no more clusters or no more bytes left)
  } while (!file->eof || bytes_left > 0);

  // Free temporary cluster buffer
  kfree ((Uint32)cluster);
}


/**
 *
 */
fat12_file_t *obs_fat12_find_subdir (vfs_mount_t *mount, fat12_file_t *file, const char *dirName) {
  char dosName[12];
  int i;

  // Convert filename to FAT name
  fat12_convert_c_to_dos_filename (dirName, dosName);

  // Do as long as we have file entries (always padded on sector which is always divved by 512)
  while (! file->eof) {
    char buf[512];
    obs_fat12_read_file_data (mount, file, buf, 512);

    // Browse all entries
    fat12_dirent_t *dirptr = (fat12_dirent_t *)&buf;
    for (i=0; i!=512 / sizeof (fat12_dirent_t); i++) {
      // Filename is 0, no more entries (only padding and this is the last sector
      if (dirptr->Filename[0] == 0) return NULL;

      // check if this is the correct directory entry
      if (strncmp ((char *)dirptr->Filename, dirName, 11) == 0) {
        // Create tmpfile
        fat12_file_t *tmpfile = (fat12_file_t *)kmalloc (sizeof(fat12_file_t));

        // Set info
        strcpy (tmpfile->name, dirName);
        tmpfile->eof = 0;
        tmpfile->length = dirptr->FileSize;
        tmpfile->offset = 0;
        tmpfile->flags = (dirptr->Attrib == 0x10) ? FS_DIRECTORY : FS_FILE;
        tmpfile->currentCluster = dirptr->FirstCluster;
        tmpfile->currentClusterOffset = 0;
        return tmpfile;
      }
      // Try next directory entry
      dirptr++;
    }
  }

  // Nothing found
  return NULL;
}