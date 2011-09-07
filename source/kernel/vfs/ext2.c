
/******************************************************************************
 *
 *  File        : ext2.c
 *  Description : Ext2 filesystem
 *
 *****************************************************************************/

#include "kernel.h"
#include "kmem.h"
#include "vfs.h"
#include "vfs/ext2.h"
#include "drivers/floppy.h"
#include "drivers/ide.h"

// File operations
static struct vfs_fileops ext2_fileops = {
    .read = ext2_read, .write = ext2_write,
    .open = ext2_open, .close = ext2_close,
    .readdir = ext2_readdir, .finddir = ext2_finddir
};

// Mount operations
static struct vfs_mountops ext2_mountops = {
    .mount = ext2_mount, .umount = ext2_umount
};

// Global structure
static vfs_info_t ext2_vfs_info = { .tag = "ext2",
                                    .name = "EXT2 File System",
                                    .mountops = &ext2_mountops
                                  };

// Root supernode
static vfs_node_t ext2_supernode = {
  .inode_nr = 0,
  .name = "/",
  .owner = 0,
  .length = 0,
  .flags = FS_DIRECTORY,
  .major_num = 0,
  .minor_num = 0,
  .fileops = &ext2_fileops,
};

vfs_dirent_t dirent;   // Entry that gets returned by ext2_readdir
vfs_node_t filenode;   // Entry that gets returned by ext2_finddir



/**
 * Called when a device that holds a EXT2 image gets mounted onto a mount_point
 */
vfs_node_t *ext2_mount (struct vfs_mount *mount, device_t *dev, const char *path) {
}

/**
 * Called when a mount_point gets unmounted
 */
void ext2_umount (struct vfs_mount *mount) {
}



/**
 *
 */
Uint32 ext2_read (vfs_node_t *node, Uint32 offset, Uint32 size, char *buffer) {

}

/**
 *
 */
Uint32 ext2_write (vfs_node_t *node, Uint32 offset, Uint32 size, char *buffer) {
  return 0;
//  return node->block->write (node->block->major, node->block->minor, offset, size, buffer);
}

/**
 *
 */
void ext2_open (vfs_node_t *node) {
//  node->block->open (node->block->major, node->block->minor);
}

/**
 *
 */
void ext2_close (vfs_node_t *node) {
//  node->block->close (node->block->major, node->block->minor);
}


/**
 * Initialises the ext2 on current drive
 */
void ext2_init (void) {
  // Register filesystem to the VFS
  vfs_register_filesystem (&ext2_vfs_info);
}


vfs_node_t *ext2_finddir (vfs_node_t *node, const char *name) {
}

vfs_dirent_t *ext2_readdir (vfs_node_t *node, Uint32 index) {
}