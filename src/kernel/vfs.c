MOUNTING MUST BE IMPLEMENTED CORRECTLY... STILL THINKING ABOUT IT


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
 * // Create VFS root entry point.
 */
void vfs_create_root_node () {
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

/**
 * Return 1 when filesystem is registered (FAT12, CYBFS etc). 0 otherwise
 */
int vfs_is_registered (const char *tag) {
  int i;

  // Scan all fs slots
  for (i=0; i!=VFS_MAX_FILESYSTEMS; i++) {
    if (strcmp (tag, vfs_systems[i].tag) == 0) return 1;
  }

  // Not found
  return 0;
}

/**
 * Registers a new filesystem to the VFS
 */
int vfs_register_filesystem (vfs_info_t *info) {
  int i;

  // Scan all fs slots
  for (i=0; i!=VFS_MAX_FILESYSTEMS; i++) {
    // Already enabled, try next slot
    if (vfs_systems[i].tag[0]) continue;

    memcpy (&vfs_systems[i], info, sizeof (vfs_info_t));
    return 1;
  }

  // No more room :(
  return 0;
}

/**
 * Unregisters a filesystem by tagname
 */
int vfs_unregister_filesystem (const char *tag) {
  int i;

  /* @TODO: see if we have mountpoints present that uses this filesystem. If so, we cannot
   * disable this filesystem */

  for (i=0; i!=VFS_MAX_FILESYSTEMS; i++) {
    if (strcmp (tag, vfs_systems[i].tag) == 0) {
      // disable filesystem. This is now a free slot again..
      vfs_systems[i].tag[0] = 0;
      return 1;
    }
  }

  // Nothing found
  return 0;
}


/**
 *
 */
void vfs_init (void) {
  // Clear all fs slot data
  memset (vfs_systems, 0, sizeof (vfs_systems));

  vfs_create_root_node ();
}


/**
 *
 */
int sys_mount (const char *device, const char *mount_point, const char *fs_type) {
  // Cannot mount if mount_point is not a directory
  if ((mount_point->flags & 0x7) != FS_DIRECTORY) return 0;

  // Check if filesystem is registered
  if (! vfs_is_registered (fs_type)) return 0;

  kprintf ("Mounting %s onto %s as a %s filesystem\n", device, mount_point, fs_type);

  // Remove mount flag
  mount_point->flags |= FS_MOUNTPOINT;
  mount_point->old_fileops = mount_point->fileops;
  mount_point->fileops =

  return 1;
}