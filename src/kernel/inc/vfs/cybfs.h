/******************************************************************************
 *
 *  File        : cybfs.h
 *  Description : CybFS in-memory filesystem
 *
 *****************************************************************************/
#ifndef __VFS_CYBFS_H__
#define __VFS_CYBFS_H__

#define CYBFS_MAX_FILES        256

#define CYBFS_TYPE_FILE        0x01
#define CYBFS_TYPE_DIRECTORY   0x02
#define CYBFS_TYPE_DEVICE      0x04

typedef struct {
    Uint8  inode_nr;          // Inode (or index number)
    Uint8  parent_inode_nr;   // Parent inode (if in subdir, otherwise 0)
    char   name[128];         // Name of file
    Uint8  type;              // Type (bitwise CYBFS_TYPE_*)
    Uint8  device_major_num;  // Device major number (if applicable)
    Uint8  device_minor_num;  // Device minor number (if applicable)
    Uint32 length;            // Length of data
    char   *data;             // Pointer to data
} cybfs_file_t;

  cybfs_file_t cybfs_nodes[CYBFS_MAX_FILES-1];   // Max 256 files (inc subdirs)


  extern vfs_fileops_t cybfs_fops;

  void cybfs_init ();
  Uint32 cybfs_read (vfs_node_t *node, Uint32 offset, Uint32 size, char *buffer);
  Uint32 cybfs_write (vfs_node_t *node, Uint32 offset, Uint32 size, char *buffer);
  void cybfs_open (vfs_node_t *node);
  void cybfs_close (vfs_node_t *node);
  vfs_dirent_t *cybfs_readdir (vfs_node_t *node, Uint32 index);
  vfs_node_t *cybfs_finddir (vfs_node_t *node, const char *name);

#endif // __VFS_CYBFS_H__