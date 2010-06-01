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

typedef struct {
  char          enabled;        // 0 = disabled, 1 = enabled
  int           mount_index;    // Mounted onto which other mount
  inode_t       inode_nr;       // Inode nr of the mounted directory
  device_t      *device;        // Which device is mounted onto here
  vfs_info_t    info;           // File operations for this mount
} vfs_mount_t;

vfs_mount_t vfs_mount_table[VFS_MAX_MOUNTS];    // Mount table


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

  node->fileops = &cybfs_fileops;    // Handled by CybFS

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
      // System is still mounted at least 1 time
      if (vfs_systems[i].ref_count > 0) return 0;

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
int vfs_get_vfs (const char *tag, vfs_info_t *info) {
  int i;

  // Browse all registered file systems
  for (i=0; i!=VFS_MAX_FILESYSTEMS; i++) {
    // Is it taken? (ie: tag is not empty)
    if (strcmp (tag, vfs_systems[i].tag) == 0) {
      info = &vfs_systems[i];
      return 1;
    }
  }

  // Not found
  return 0;
}

/**
 *
 */
void vfs_init (void) {
  // Clear all fs slot data
  memset (vfs_systems, 0, sizeof (vfs_systems));

  // Clear all mount tables
  memset (vfs_mount_table, 0, sizeof (vfs_mount_table));

  vfs_create_root_node ();
}


/**
 *
 */
vfs_node_t *vfs_get_node_from_path (const char *path, vfs_node_t *cur_node) {
  char component[255];
  char *c = (char *)path;
  int cs;

  kprintf ("vfs_get_node_from_path('%s')\n", path);

  // This is a absolute path, start from root node (don't care
  if (*c == '/') {
    kprintf ("Starting from <root>\n");
    cur_node = vfs_root;    // This is the root node, start from here
  }

  // From this point, it's relative again
  while (*c) {
    if (*c == '/') c++;   // Skip directory separator if one is found (including root separator)

    /* Cur_node MUST be a directory, since the next thing we read is a directory component
     * and we still have something left on the path */
    if ((cur_node->flags & 0x7) != FS_DIRECTORY) {
      kprintf ("component is not a directory\n\n");
      return NULL;
    }

    // Find next directory component (or to end of string)
    for (cs = 0 ; *(c+cs) != '/' && *(c+cs) != 0; cs++);

    // Separate this directory component
    strncpy (component, c, cs);

    // Increase to next entry (starting at the directory separator)
    c+=cs;

    kprintf ("Next component: '%s' (from inode %d)\n", component, cur_node->inode_nr);

    // Dot directory, skip since we stay on the same node
    if (strcmp (component, ".") == 0) continue;   // We could vfs_finddir deal with it, or do it ourself, we are faster

    // Find the entry if it exists
    cur_node = vfs_finddir (cur_node, component);
    if (! cur_node) {
      kprintf ("component not found\n\n");
      return NULL;   // Cannot find node... error :(
    }

  }

  kprintf ("All done.. returning vfs inode: %d\n\n", cur_node->inode_nr);

  return cur_node;
}




/**
 *
 */
int sys_umount (const char *mount_point) {
  kpanic ("Unmounting %s \n", mount_point);

  // if (vfs_mount_table[i].vfs_info.ref_count == 0) kpanic ("Something is wrong. We have a mounted system but the vfs_systems table says it's not mounted. halting.");
  // vfs_mount_table[i].vfs_info.ref_count--;

  return 0;
}


/**
 *
 */
int sys_mount (const char *device_path, const char *mount_path, const char *fs_type) {
  int i;

  // Check mount point
  vfs_node_t *mount_node = vfs_get_node_from_path (mount_path, vfs_root);
  if (! mount_node) return 0;   // Path not found

  // Cannot mount if mount_point is not a directory
  if ((mount_node->flags & 0x7) != FS_DIRECTORY) return 0;

  // This path is already mounted
  if ( (mount_node->flags & FS_MOUNTPOINT) == FS_MOUNTPOINT) return 0;

  // Check device
  vfs_node_t *dev_node = vfs_get_node_from_path (device_path, vfs_root);
  if (! dev_node) return 0;   // Path not found

  // Cannot mount device if it's not a block device
  if ((mount_node->flags & 0x7) != FS_BLOCKDEVICE) return 0;

  // Browse mount table, find first free slot
  for (i=0; i!=VFS_MAX_MOUNTS; i++) {
    // Find free slot
    if (vfs_mount_table[i].enabled) continue;

    // Everything is ok now...
    kprintf ("Mounting %s onto %s as a %s filesystem\n", device_path, mount_path, fs_type);

    vfs_mount_table[i].mount_index = 0;
    vfs_mount_table[i].inode_nr = mount_node->inode_nr;
    if (! device_get_device (dev_node->majorNum, dev_node->minorNum, &vfs_mount_table[i].device)) {
      // Cannot find the device registered
      return 0;
    }

/*
    if (! vfs_get_vfs (fs_type, &vfs_mount_table[i].vfs_info) {
      // Cannot find registered file system
      return 0;
    }

    // Increase the reference counter for this filesystem
    vfs_mount_table[i].vfs_info.ref_count++;
*/
    // This entry is enabled (@TODO: do this atomic / irq_disabled)
    vfs_mount_table[i].enabled = 1;

    return 1;
  }

  // Could not find a free slot
  return 0;
}


