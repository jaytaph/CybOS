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



#define EXT2_BLOCKGROUPDESCRIPTOR_BLOCK       2
#define EXT2_BLOCKBITMAP_BLOCK                3
#define EXT2_INODEBITMAP_BLOCK                4
#define EXT2_INODETABLE_BLOCK                 5

// Reserved inode numbers
#define EXT2_BAD_INO                       1
#define EXT2_ROOT_INO                      2
#define EXT2_ACL_IDX_INO                   3
#define EXT2_ACL_DATA_INO                  4
#define EXT2_BOOT_LOADER_INO               5
#define EXT2_UNDEL_DIR_INO                 6


// -- file format --
#define EXT2_S_IFSOCK	0xC000	// socket
#define EXT2_S_IFLNK	0xA000	// symbolic link
#define EXT2_S_IFREG	0x8000	// regular file
#define EXT2_S_IFBLK	0x6000	// block device
#define EXT2_S_IFDIR	0x4000	// directory
#define EXT2_S_IFCHR	0x2000	// character device
#define EXT2_S_IFIFO	0x1000	// fifo

// -- process execution user/group override --
#define EXT2_S_ISUID	0x0800	// Set process User ID
#define EXT2_S_ISGID	0x0400	// Set process Group ID
#define EXT2_S_ISVTX	0x0200	// sticky bit

// -- access rights --
#define EXT2_S_IRUSR	0x0100	// user read
#define EXT2_S_IWUSR	0x0080	// user write
#define EXT2_S_IXUSR	0x0040	// user execute
#define EXT2_S_IRGRP	0x0020	// group read
#define EXT2_S_IWGRP	0x0010	// group write
#define EXT2_S_IXGRP	0x0008	// group execute
#define EXT2_S_IROTH	0x0004	// others read
#define EXT2_S_IWOTH	0x0002	// others write
#define EXT2_S_IXOTH	0x0001	// others execute

#pragma pack(1)
typedef struct {
    Uint32      inodeCount;
    Uint32      blockCount;
    Uint32      reservedBlockCount;
    Uint32      unallocatedBlockCount;
    Uint32      unalloactedInodeCount;
    Uint32      firstDataBlock;
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


#pragma pack(1)
typedef struct {
    Uint32      inode_nr;
    Uint16      rec_len;
    Uint8       name_len;
    Uint8       file_type;
    char        name[];
} ext2_dir_t;


typedef struct {
    // Pointers to pre-read items
    ext2_superblock_t       *superblock;
    ext2_blockdescriptor_t  *block_descriptor;
    void                    *block_bitmap;
    void                    *inode_bitmap;

    // Precalced values
    Uint32                  block_size;
    Uint32                  group_count;
    Uint32                  sectors_per_block;
    Uint32                  first_group_start;
    Uint32                  group_descriptor_count;
} ext2_info_t;



  void ext2_init (void);
  Uint32 ext2_read (vfs_node_t *node, Uint32 offset, Uint32 size, char *buffer);
  Uint32 ext2_write (vfs_node_t *node, Uint32 offset, Uint32 size, char *buffer);
  void ext2_open (vfs_node_t *node);
  void ext2_close (vfs_node_t *node);
  int ext2_readdir (vfs_node_t *node, Uint32 index, vfs_dirent_t *target_dirent);
  int ext2_finddir (vfs_node_t *node, const char *name, vfs_node_t *target_node);

  vfs_node_t *ext2_mount (struct vfs_mount *mount, device_t *dev, const char *path);
  void ext2_umount (struct vfs_mount *mount);

#endif // __VFS_EXT2_H__

