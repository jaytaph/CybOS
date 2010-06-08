/******************************************************************************
 *
 *  File        : devfs.h
 *  Description : DevFS in-memory driver meta filesystem
 *
 *****************************************************************************/
#ifndef __VFS_DEVFS_H__
#define __VFS_DEVFS_H__

#define DEVFS_MAX_FILES        256

#define DEVFS_TYPE_DIRECTORY   0x02
#define DEVFS_TYPE_BLOCK_DEV   0x03
#define DEVFS_TYPE_CHAR_DEV    0x04


typedef struct {
    Uint8  inode_nr;          // Inode (or index number)
    Uint8  parent_inode_nr;   // Parent inode (if in subdir, otherwise 0)
    char   name[128];         // Name of file
    Uint8  type;              // Type
    Uint8  major_num;         // Device major number (if applicable)
    Uint8  minor_num;         // Device minor number (if applicable)
} devfs_file_t;

  devfs_file_t devfs_nodes[DEVFS_MAX_FILES-1];   // Max 256 files can be added to a CybFS system (inc subdirs)

  void devfs_init ();
  vfs_dirent_t *devfs_readdir (struct vfs_mount *mount, vfs_node_t *node, Uint32 index);
  vfs_node_t *devfs_finddir (struct vfs_mount *mount, vfs_node_t *node, const char *name);
  void devfs_mknod (struct vfs_mount *mount, vfs_node_t *node, const char *name, char device_type, Uint8 major_node, Uint8 minor_node);

  vfs_node_t *devfs_mount (struct vfs_mount *mount, device_t *dev, const char *path);
  void devfs_umount (struct vfs_mount *mount);

#endif // __VFS_DEVFS_H__