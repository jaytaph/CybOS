/******************************************************************************
 *
 *  File        : fat12.h
 *  Description : Fat12 filesystem
 *
 *****************************************************************************/
#ifndef __VFS_FAT12_H__
#define __VFS_FAT12_H__

#include "drivers/floppy.h"

// Fat structure is array of chars
typedef char * fat12_fat_t;

#pragma pack(1)
typedef struct {
    Uint8   jmp_command[3];

    Uint8   OEMName[8];
    Uint16  BytesPerSector;
    Uint8   SectorsPerCluster;
    Uint16  ReservedSectors;
    Uint8   NumberOfFats;
    Uint16  NumDirEntries;
    Uint16  NumSectors;
    Uint8   Media;
    Uint16  SectorsPerFat;
    Uint16  SectorsPerTrack;
    Uint16  HeadsPerCyl;
    Uint32  HiddenSectors;
    Uint32  LongSectors;

    Uint32  SectorsPerFat32;   //sectors per FAT
    Uint16  Flags;             //flags
    Uint16  Version;           //version
    Uint32  RootCluster;       //starting root directory
    Uint16  InfoCluster;
    Uint16  BackupBoot;        //location of bootsector copy
    Uint16  Reserved[6];
} fat12_bpb_t;

#pragma pack(1)
typedef struct {
  Uint8   Filename[8];           //filename
  Uint8   Ext[3];                //extension (8.3 filename format)
  Uint8   Attrib;                //file attributes
  Uint8   Reserved;
  Uint8   TimeCreatedMs;         //creation time
  Uint16  TimeCreated;
  Uint16  DateCreated;           //creation date
  Uint16  DateLastAccessed;
  Uint16  FirstClusterHiBytes;
  Uint16  LastModTime;           //last modification date/time
  Uint16  LastModDate;
  Uint16  FirstCluster;          //first cluster of file data
  Uint32  FileSize;              //size in bytes
} fat12_dirent_t;


typedef struct {
  char        name[255];              // name of the file
  Uint8       eof;                    // 0 = more data, 1 = no more data left
  Uint32      length;                 // length of the file
  Uint32      offset;                 // Offset in the file (0= start)
  Uint32      flags;                  // File flags
  Uint32      currentCluster;         // Current cluser
  Uint16      currentClusterOffset;   // Offset in the cluster
} fat12_file_t;

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

  fat12_bpb_t *bpb;            // Pointer to Bios Parameter Block
  fat12_fat_t *fat;            // Pointer to (primary) FAT table
} fat12_fatinfo_t;


  void fat12_init (void);
  Uint32 fat12_read (struct vfs_mount *mount, vfs_node_t *node, Uint32 offset, Uint32 size, char *buffer);
  Uint32 fat12_write (struct vfs_mount *mount, vfs_node_t *node, Uint32 offset, Uint32 size, char *buffer);
  void fat12_open (struct vfs_mount *mount, vfs_node_t *node);
  void fat12_close (struct vfs_mount *mount, vfs_node_t *node);
  vfs_dirent_t *fat12_readdir (struct vfs_mount *mount, vfs_node_t *node, Uint32 index);
  vfs_node_t *fat12_finddir (struct vfs_mount *mount, vfs_node_t *node, const char *name);

  vfs_node_t *fat12_mount (struct vfs_mount *mount, device_t *dev, const char *path);
  void fat12_umount (struct vfs_mount *mount);

#endif // __VFS_FAT12_H__