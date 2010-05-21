/******************************************************************************
 *
 *  File        : fat12.c
 *  Description : Fat12 filesystem
 *
 *  TODO: Wrong things happend when clustersize != sectorsize != 512!!!
 *
 *****************************************************************************/

#include "vfs.h"
#include "vfs/fat12.h"
#include "kernel.h"
#include "kmem.h"
#include "drivers/floppy.h"

fat12_fatinfo_t fat12_info;   // FAT information table (offsets and sizes)

fs_dirent_t dirent;   // Entry that gets returned by fat12_readdir
fs_node_t filenode;   // Entry that gets returned by fat12_finddir

// Forward defines
void fat12_convert_dos_to_c_filename (char *longName, const char *dosName);
void fat12_convert_c_to_dos_filename (const char *longName, char *dosName);
Uint16 fat12_get_next_cluster (Uint16 cluster);

/**
 *
 */
Uint32 fat12_read (fs_node_t *node, Uint32 offset, Uint32 size, char *buffer) {
    return node->block->read (node->block->major, node->block->minor, offset, size, buffer);
}

/**
 *
 */
Uint32 fat12_write (fs_node_t *node, Uint32 offset, Uint32 size, char *buffer) {
  return node->block->write (node->block->major, node->block->minor, offset, size, buffer);
}

/**
 *
 */
void fat12_open (fs_node_t *node) {
  node->block->open (node->block->major, node->block->minor);
}

/**
 *
 */
void fat12_close (fs_node_t *node) {
  node->block->close (node->block->major, node->block->minor);
}


/**
 *
 */
fs_dirent_t *fat12_readdir (fs_node_t *node, Uint32 index) {
  int i;

  // Check if it's a directory
  if ((node->flags & 0x7) != FS_DIRECTORY) return NULL;

  // Every sector holds this many directories
  int dirsPerSector = fat12_info.bpb->BytesPerSector / 32;
  int sectorNeeded = index / dirsPerSector;   // Sector we need (@TODO: problem when sector != cluster)
  int indexNeeded = index % dirsPerSector;    // N'th entry in this sector needed

  // Create entry that holds 1 sector
  fat12_dirent_t *direntbuf = (fat12_dirent_t *)kmalloc (fat12_info.bpb->BytesPerSector);

  // Do we need to read the root directory?
  if (node->inode_nr == 0) {
    Uint32 offset = (fat12_info.rootOffset+sectorNeeded) * fat12_info.bpb->BytesPerSector;
    node->block->read (node->block->major, node->block->minor, offset, fat12_info.bpb->BytesPerSector, direntbuf);

  } else {
    // First start cluster (which is the INODE number :P), and seek N'th cluster
    Uint16 cluster = node->inode_nr;
    for (i=0; i!=sectorNeeded; i++) cluster = fat12_get_next_cluster (cluster);

    // Read this cluster
    Uint32 offset = (fat12_info.dataOffset+cluster) * fat12_info.bpb->BytesPerSector;
    node->block->read (node->block->major, node->block->minor, offset, fat12_info.bpb->BytesPerSector, direntbuf);
  }

  // Seek correcy entry
  fat12_dirent_t *direntbufptr = direntbuf;
  for (i=0; i!=indexNeeded; i++) direntbufptr+=sizeof (fat12_dirent_t);

  // No more entries when first char of filename is 0
  if (direntbufptr->Filename[0] == 0) {
    kfree ((Uint32)direntbuf);
    return NULL;
  }

  // Create returning value
  fat12_convert_dos_to_c_filename (dirent.name, (char *)direntbufptr->Filename);
  dirent.inode_nr = direntbufptr->FirstCluster;

  kfree ((Uint32)direntbuf);
  return &dirent;
}


/**
 * Returns:
 *   0 = end of buffer, need more data
 *   1 = found item. filenode is filled
 *   2 = no more entries
 */
int fat12_parseDirectorySector (fat12_dirent_t *direntbuf, fs_node_t *node, const char *dosName) {
//  kprintf ("fat12_parseDirectorySector\n");
  fat12_dirent_t *direntbufptr = direntbuf;
  int j;

  // Browse all entries in the sector
  for (j=0; j!=fat12_info.numRootEntriesPerSector; j++, direntbufptr++) {
    // No more entries when first char of filename is 0
    if (direntbufptr->Filename[0] == 0) return 2;

//    kprintf ("Equal: '%s' and '%s' \n", dosName, direntbufptr->Filename);

    // Is this the correct file?
    if (strncmp (dosName, (char *)direntbufptr->Filename, 11) == 0) {
      // Copy parent data (so we don't have to set functions etc)
      memcpy (&filenode, node, sizeof (fs_node_t));

      // Create returning value
      filenode.inode_nr = direntbufptr->FirstCluster;
      fat12_convert_dos_to_c_filename (filenode.name, (char *)direntbufptr->Filename);
      filenode.owner = 0;
      filenode.length = direntbufptr->FileSize;
      filenode.flags = (direntbufptr->Attrib == 0x10) ? FS_DIRECTORY : FS_FILE;
      filenode.ptr = 0;

      return 1;
    }
  } // for entries

  // All entries parsed, more entries probably available and not yet found
  return 0;
}


/**
 *
 */
fs_node_t *fat12_finddir (fs_node_t *node, char *name) {
  char dosName[12];
  int i;

  // Check if it's a directory
  if ((node->flags & 0x7) != FS_DIRECTORY) return NULL;

  // Convert filename to FAT name
  fat12_convert_c_to_dos_filename (name, dosName);

  // Allocate buffer memory for directory entry
  fat12_dirent_t *direntbuf = (fat12_dirent_t *)kmalloc (fat12_info.bpb->BytesPerSector);

  // Do we need to read the root directory?
  if (node->inode_nr == 0) {
    // Read all root directory sectors until we reach the correct one or end of sectors

    for (i=0; i!=fat12_info.rootSizeSectors; i++) {
      // Read directory sector
      Uint32 offset = (fat12_info.rootOffset+i) * fat12_info.bpb->BytesPerSector;
      node->block->read (node->block->major, node->block->minor, offset, fat12_info.bpb->BytesPerSector, direntbuf);

      int ret = fat12_parseDirectorySector (direntbuf, node, dosName);
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
      Uint32 offset = (fat12_info.dataOffset+cluster) * fat12_info.bpb->BytesPerSector;
      node->block->read (node->block->major, node->block->minor, offset, fat12_info.bpb->BytesPerSector, direntbuf);

      int ret = fat12_parseDirectorySector (direntbuf, node, dosName);
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
      cluster = fat12_get_next_cluster (cluster);
//      kprintf ("New Cluster: %04x\n", cluster);
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
  while (i < 8 && dosName[i] != ' ') {
    longName[j++] = dosName[i++];
  }

  // Add extension if any is available
  if (dosName[8] != ' ') {
    longName[j++] = '.';

    i=8;
    while (i < 11 && dosName[i] != ' ') {
      longName[j++] = dosName[i++];
    }
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
 * Searches for a directory name inside the FAT's root-directory entry
 */
fat12_dirent_t *obs_fat12_search_root_directory (const char *dirName) {
  char dosName[12];
  int i,j;

  // Convert filename to FAT name
  fat12_convert_c_to_dos_filename (dirName, dosName);

  kflush ();
//  kprintf ("LONG2DOS : '%s'", dosName);

  // Browse all root directory sectors
  fat12_dirent_t *direntbuf = (fat12_dirent_t *)kmalloc (fat12_info.bpb->BytesPerSector);
  fat12_dirent_t *direntbufptr = direntbuf;
  for (i=0; i!=fat12_info.rootSizeSectors; i++) {
    // Read directory sector
    Uint32 offset = (fat12_info.rootOffset+i) * fat12_info.bpb->BytesPerSector;
    node->block->read (node->block->major, node->block->minor, offset, fat12_info.bpb->BytesPerSector, direntbuf);

    // Browse all entries in the sector
    for (j=0; j!=fat12_info.numRootEntriesPerSector; j++,direntbufptr++) {
      // No more entries when first char of filename is 0
      if (direntbufptr->Filename[0] == 0) {
        kfree ((Uint32)direntbuf);
        return NULL;
      }

      // Is this the correct file?
      if (strncmp (dosName, (char *)direntbufptr->Filename, 11) == 0) {
        // Create returning value
        fat12_dirent_t *dirent = kmalloc (sizeof (fat12_dirent_t));

        // Copy the data over to return buffer
        memcpy (dirent, direntbufptr, sizeof (fat12_dirent_t));

        // Free buffer
        kfree ((Uint32)direntbuf);
        return dirent;
      }
    } // for entries
  } // for sectors

  // Free buffer
  kfree ((Uint32)direntbuf);
  return NULL;
}


/**
 * Allocates and initalizes the FAT12's entry root-node
 */
fs_node_t *fat12_init_root_node (void) {
  // Allocate root node
  fs_node_t *root_node = (fs_node_t *)kmalloc (sizeof (fs_node_t));

  // Copy and set data
  strncpy (root_node->name, "root", 4);   // Never referenced, so name does not matter
  root_node->owner = 0;
  root_node->length = 0;
  root_node->ptr = 0;

  // inode 0 indicates this 'file' is the root directory entry
  root_node->inode_nr = 0;

  // Root node is obviously a directory
  root_node->flags = FS_DIRECTORY;

  // This node uses FAT12 I/O operations
  root_node->read = &fat12_read;
  root_node->write = &fat12_write;
  root_node->open = &fat12_open;
  root_node->close = &fat12_close;
  root_node->readdir = &fat12_readdir;
  root_node->finddir = &fat12_finddir;

  // Return the node
  return root_node;
}



/**
 * Initialises the FAT12 on current drive
 */
void fat12_init () {
  // Allocate root node for this filesystem
  fs_node_t *fat12_root_node = fat12_init_root_node ();

  // Zero out fat table
  memset (&fat12_info, 0, sizeof (fat12_fatinfo_t));

  // Allocate and read BPB
  fat12_info.bpb = (fat12_bpb_t *)kmalloc (sizeof (fat12_bpb_t));

  // Read first sector
  Uint32 offset = 0;
  node->block->read (node->block->major, node->block->minor, offset, fat12_info.bpb->BytesPerSector, (char *)fat12_info.bpb);



/*
    kprintf ("OEMName           %c%c%c%c%c%c%c%c\n", bpb->OEMName[0],bpb->OEMName[1],bpb->OEMName[2],bpb->OEMName[3],bpb->OEMName[4],bpb->OEMName[5],bpb->OEMName[6],bpb->OEMName[7]);
    kprintf ("BytesPerSector    %04x\n", bpb->BytesPerSector);
    kprintf ("SectorsPerCluster %02x\n", bpb->SectorsPerCluster);
    kprintf ("ReservedSectors   %04x\n", bpb->ReservedSectors);
    kprintf ("NumberOfFats      %02x\n", bpb->NumberOfFats);
    kprintf ("NumDirEntries     %04x\n", bpb->NumDirEntries);
    kprintf ("NumSectors        %04x\n", bpb->NumSectors);
    kprintf ("Media             %02x\n", bpb->Media);
    kprintf ("SectorsPerFat     %04x\n", bpb->SectorsPerFat);
    kprintf ("SectorsPerTrack   %04x\n", bpb->SectorsPerTrack);
    kprintf ("HeadsPerCyl       %04x\n", bpb->HeadsPerCyl);
    kprintf ("HiddenSectors     %08x\n", bpb->HiddenSectors);
    kprintf ("LongSectors       %08x\n", bpb->LongSectors);
*/

  // Set global FAT information as read and calculated from the bpb
  fat12_info.numSectors              = fat12_info.bpb->NumSectors;
  fat12_info.fatOffset               = fat12_info.bpb->ReservedSectors + fat12_info.bpb->HiddenSectors;
  fat12_info.fatSizeBytes            = fat12_info.bpb->BytesPerSector * fat12_info.bpb->SectorsPerFat;
  fat12_info.fatSizeSectors          = fat12_info.bpb->SectorsPerFat;
  fat12_info.fatEntrySizeBits        = 8;
  fat12_info.numRootEntries          = fat12_info.bpb->NumDirEntries;
  fat12_info.numRootEntriesPerSector = fat12_info.bpb->BytesPerSector / 32;
  fat12_info.rootOffset              = (fat12_info.bpb->NumberOfFats * fat12_info.bpb->SectorsPerFat) + 1;
  fat12_info.rootSizeSectors         = (fat12_info.bpb->NumDirEntries * 32 ) / fat12_info.bpb->BytesPerSector;
  fat12_info.dataOffset              = fat12_info.rootOffset + fat12_info.rootSizeSectors - 2;

/*
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
*/

  // Read (primary) FAT table
  int i;
  fat12_info.fat = (fat12_fat_t *)kmalloc (fat12_info.fatSizeBytes);
  for (i=0; i!=fat12_info.fatSizeSectors; i++) {
    Uint32 offset = (fat12_info.fatOffset+i) * fat12_info.bpb->BytesPerSector;
    node->block->read (node->block->major, node->block->minor, offset, fat12_info.bpb->BytesPerSector, (char *)fat12_info.fat+(i*512));
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
    Uint32 offset = (fat12_info.rootOffset+i) * fat12_info.bpb->BytesPerSector;
    node->block->read (node->block->major, node->block->minor, offset, fat12_info.bpb->BytesPerSector, direntbuf);

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

  return fat12_root_node;
}


/**
 * Finds the next cluster inside the FAT table
 */
Uint16 fat12_get_next_cluster (Uint16 cluster) {
  Uint32 offset = cluster + (cluster / 2);
  Uint16 next_cluster = *( (Uint16 *)(fat12_info.fat + offset));

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
void fat12_read_file_data (fat12_file_t *file, char *buffer, Uint32 byte_count) {
  // No file, no read
  if (! file) return;

  char *bufptr = buffer;

  // Number of bytes
  Uint32 bytes_left = byte_count;

  // Buffer to hold 1 cluster
  char *cluster = (char *)kmalloc (fat12_info.bpb->BytesPerSector * fat12_info.bpb->SectorsPerCluster);

  do {
    // Read current cluster  @TODO: error when cluster size != sector size!
    Uint32 offset = file->currentCluster * fat12_info.bpb->BytesPerSector;
    node->block->read (node->block->major, node->block->minor, offset, fat12_info.bpb->BytesPerSector, cluster);

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
      file->currentCluster = fat12_get_next_cluster (file->currentCluster);

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
fat12_file_t *fat12_find_subdir (fat12_file_t *file, const char *dirName) {
  char dosName[12];
  int i;

  // Convert filename to FAT name
  fat12_convert_c_to_dos_filename (dirName, dosName);

  // Do as long as we have file entries (always padded on sector which is always divved by 512)
  while (! file->eof) {
    char buf[512];
    fat12_read_file_data (file, buf, 512);

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