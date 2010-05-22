/******************************************************************************
 *
 *  File        : vfs.c
 *  Description : Virtual file system
 *
 *****************************************************************************/

#include "kernel.h"
#include "kmem.h"
#include "vfs.h"
#include "vfs/cybfs.h"

vfs_node_t *vfs_root = NULL;      // Root of the filesystem


/**
 *
 */
Uint32 vfs_read (vfs_node_t *node, Uint32 offset, Uint32 size, char *buffer) {
  // Check if it's a directory
  if ((node->flags & 0x7) != FS_FILE) return 0;

  if (! node->fileops || ! node->fileops->read) return 0;
  return node->fileops->read (node, offset, size, buffer);
}

/**
 *
 */
Uint32 vfs_write (vfs_node_t *node, Uint32 offset, Uint32 size, char *buffer) {
//    return (node->fileops.write == NULL) ? NULL : node->fileops.write (node, offset, size, buffer);
  return 0;
}

/**
 *
 */
void vfs_open(vfs_node_t *node) {
//    return (node->fileops.open == NULL) ? NULL : node->fileops.open (node);
}

/**
 *
 */
void vfs_close(vfs_node_t *node) {
//    return (node->fileops.close == NULL) ? NULL : node->fileops.close (node);
}

/**
 *
 */
struct dirent *vfs_readdir(vfs_node_t *node, Uint32 index) {
//  kprintf ("vfs_readdir (%s, %d)\n", node->name, index);

  // Check if it's a directory
  if ((node->flags & 0x7) != FS_DIRECTORY) return NULL;

  if (! node->fileops || ! node->fileops->readdir) return NULL;
  return node->fileops->readdir (node, index);
}

/**
 *
 */
vfs_node_t *vfs_finddir(vfs_node_t *node, const char *name) {
//  kprintf ("vfs_finddir (%s, '%s')\n", node->name, name);

    // Check if it's a directory
  if ((node->flags & 0x7) != FS_DIRECTORY) return NULL;

  if (! node->fileops || ! node->fileops->finddir) return NULL;
  return node->fileops->finddir (node, name);
}

/**
 *
 */
void vfs_init (void) {
  // Create VFS root hierarchy.
  vfs_node_t *node = (vfs_node_t *)kmalloc (sizeof (vfs_node_t));

  node->inode_nr = 0;
  strcpy (node->name, "vfs root");
  node->owner = 0;
  node->length = 0;
  node->flags = FS_DIRECTORY | FS_MOUNTPOINT;
  node->majorNum = 0;
  node->minorNum = 0;

  node->fileops = &cybfs_fops;    // Handled by CybFS

  // This node is the main root
  vfs_root = node;
}