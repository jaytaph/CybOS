/******************************************************************************
 *
 *  File        : ext2.h
 *  Description : Ext2 filesystem
 *
 *****************************************************************************/
#ifndef __VFS_EXT2_H__
#define __VFS_EXT2_H__

#include "drivers/floppy.h"

//// Ext structure is array of chars
//typedef char * ext2_ext2_t;



#pragma pack(1)
typedef struct {
    Uint32      inodeCount;
    Uint32      blockCount;
    Uint32      reservedBlockCount;
    Uint32      unallocatedBlockCount;
    Uint32      unalloactedInodeCount;
    Uint32      superblockBlockNumber;
    Uint32      blockSizeLog2;
    Uint32      fragmentSizeLog2;
    Uint32      blocksInGroupCount;
    Uint32      fragmentsInGroupCount;
    Uint32      inodesInGroupCount;
    Uint32      lastMountTime;
    Uint32      lastWriteTime;
    Uint16      mountCountSinceCheck;
    Uint16      maxMountCountBeforeCheck;
    Uint16      ext2Signature;
    Uint16      filesystemState;
    Uint16      errorHandling;
    Uint16      versionMinor;
    Uint32      consistencyCheckTime;
    Uint32      intervalConsistencyCheckTime;
    Uint32      operatingSystemId;
    Uint32      versionMajor;
    Uint16      reservedBlocksUid;
    Uint16      reservedBlocksGid;
} ext2_superblock_t;

#pragma pack(1)
typedef struct {
    Uint32      firstNonReservedInode;
    Uint16      inodeSize;
    Uint16      superblockBlockgroup;
    Uint32      optionalFeatures;
    Uint32      requiredFeatures;
    Uint32      readonlyFeatures;
    char        fileSystemId[16];
    char        volumeName[16];
    char        lastMountPath[64];
    Uint32      compressionUsed;
    Uint8       preallocateFileBlockCount;
    Uint8       preallocateDirectoryBlockCount;
    Uint16      _reserved;
    char        journalId[16];
    Uint32      journalInode;
    Uint32      journalDevice;
    Uint32      orphanInodeListHead;
} ext2_superblock_extended_t;

#pragma pack(1)
typedef struct {
    Uint32      blockUsageBitmapAddress;
    Uint32      inodeUsageBitmapAddress;
    Uint32      inodeTableStart;
    Uint16      unallocatedBlockCount;
    Uint16      unallocatedInodeCount;
    Uint16      directoryCount;
    char        _reserved[14];
} ext2_blockdescriptor_t;

#pragma pack(1)
typedef struct {
    Uint16      typeAndPermissions;
    Uint16      uid;
    Uint32      sizeLow;
    Uint32      atime;
    Uint32      ctime;
    Uint32      mtime;
    Uint32      dtime;
    Uint16      gid;
    Uint16      linkCount;
    Uint32      sectorCount;
    Uint32      flags;
    Uint32      ossValue;
    Uint32      directPointerBlock[12];
    Uint32      singleIndirectPointerBlock;
    Uint32      doubleIndirectPointerBlock;
    Uint32      tripleIndirectPointerBlock;
    Uint32      generatorNumber;
    Uint32      fileACL;
    union {
        Uint32      sizeHigh;
        Uint32      directoryACL;
    };
} ext2_inode_t;



  void ext2_init (void);
  Uint32 ext2_read (vfs_node_t *node, Uint32 offset, Uint32 size, char *buffer);
  Uint32 ext2_write (vfs_node_t *node, Uint32 offset, Uint32 size, char *buffer);
  void ext2_open (vfs_node_t *node);
  void ext2_close (vfs_node_t *node);
  vfs_dirent_t *ext2_readdir (vfs_node_t *node, Uint32 index);
  vfs_node_t *ext2_finddir (vfs_node_t *node, const char *name);

  vfs_node_t *ext2_mount (struct vfs_mount *mount, device_t *dev, const char *path);
  void ext2_umount (struct vfs_mount *mount);

#endif // __VFS_EXT2_H__

