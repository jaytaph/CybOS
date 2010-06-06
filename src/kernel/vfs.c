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

vfs_mount_t vfs_mount_table[VFS_MAX_MOUNTS];    // Mount table with all mount points (@TODO: dynamically allocated or linkedlist)
vfs_system_t vfs_systems[VFS_MAX_FILESYSTEMS];  // There will be a maximum of 100 different filesystems that can be loaded (@TODO: linkedlist or dynamically allocation)

/**
 *
 */
void vfs_printnode (vfs_node_t *node) {
  kprintf ("PrintNode()\n");

  kprintf ("node->inode_nr    : %d\n", node->inode_nr);
  kprintf ("node->name        : '%s'\n", node->name);
  kprintf ("node->owner       : %d\n", node->owner);
  kprintf ("node->length      : %d\n", node->length);
  kprintf ("node->flags       : %4x\n", node->flags);
  kprintf ("node->majorNum    : %d\n", node->majorNum);
  kprintf ("node->minorNum    : %d\n", node->minorNum);
  kprintf ("\n");
}

/**
 * Returns the mount from the path (path is formatted like MOUNT:PATH), the path
 * suffix is returned in **path or points to NULL when the mount is not found.
 */
vfs_mount_t *vsf_get_mount (const char *full_path, char **path) {
  char mount[11];
  int i;

  // Scan string for : or end of string
  int len=0;
  while (full_path[len] != ':' && full_path[len] != '\0') len++;

  // Did we find a ':'?
  if (full_path[len] == '\0') return NULL;   // No : found

  // Copy mount
  strncpy ((char *)&mount, full_path, len);

  // Set path to
  *path = (char *)(full_path+len+1);

  // Browse all mounts to see if it's available
  for (i=0; i!=VFS_MAX_MOUNTS; i++) {
    if (! vfs_mount_table[i].enabled) continue;
    if (strcmp (vfs_mount_table[i].mount, mount) == 0) return &vfs_mount_table[i];
  }

  // Mount not fond
  *path = NULL;
  return NULL;
}



/**
 *
 */
Uint32 vfs_read (struct vfs_mount *mount, vfs_node_t *node, Uint32 offset, Uint32 size, char *buffer) {
  // Check if it's a directory
  if ((node->flags & 0x7) != FS_FILE) return 0;

  if (! node->fileops || ! node->fileops->read) return NULL;
  return node->fileops->read (mount, node, offset, size, buffer);
}

/**
 *
 */
Uint32 vfs_write (struct vfs_mount *mount, vfs_node_t *node, Uint32 offset, Uint32 size, char *buffer) {
  if (! node->fileops || ! node->fileops->write) return 0;
  return node->fileops->write (mount, node, offset, size, buffer);
}

/**
 *
 */
void vfs_open(struct vfs_mount *mount, vfs_node_t *node) {
  if (! node->fileops || ! node->fileops->open) return;
  node->fileops->open (mount, node);
}

/**
 *
 */
void vfs_close(struct vfs_mount *mount, vfs_node_t *node) {
  if (! node->fileops || ! node->fileops->close) return;
  node->fileops->close (mount, node);
}

/**
 *
 */
struct dirent *vfs_readdir(struct vfs_mount *mount, vfs_node_t *node, Uint32 index) {
//  kprintf ("vfs_readdir (%s, %d) : ", node->name, index);

  // Check if it's a directory
  if ((node->flags & 0x7) != FS_DIRECTORY) {
//    kprintf ("Not a dir\n");
    return NULL;
  }

  if (! node->fileops || ! node->fileops->readdir) {
//    kprintf ("Not fileops or fileops->readdir\n");
    return NULL;
  }
  return node->fileops->readdir (mount, node, index);
}

/**
 *
 */
vfs_node_t *vfs_finddir(struct vfs_mount *mount, vfs_node_t *node, const char *name) {
  // Check if it's a directory
  if ((node->flags & 0x7) != FS_DIRECTORY) return NULL;

  if (! node->fileops || ! node->fileops->finddir) return NULL;
  return node->fileops->finddir (mount, node, name);
}



/**
 * Return 1 when filesystem is registered (FAT12, CYBFS etc). 0 otherwise
 */
int vfs_is_registered (const char *tag) {
  int i;

  // Scan all fs slots
  for (i=0; i!=VFS_MAX_FILESYSTEMS; i++) {
    // Enabled and tag matched? we've got a match
    if (vfs_systems[i].enabled && strcmp (tag, vfs_systems[i].info.tag) == 0) return 1;
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
    if (vfs_systems[i].enabled) continue;

    vfs_systems[i].enabled = 1;
    memcpy (&vfs_systems[i].info, info, sizeof (vfs_info_t));
    vfs_systems[i].mount_count = 0;
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
    if (vfs_systems[i].enabled && strcmp (tag, vfs_systems[i].info.tag) == 0) {
      // System is still mounted at least 1 time
      if (vfs_systems[i].mount_count > 0) return 0;

      // disable filesystem. This is now a free slot again..
      vfs_systems[i].enabled = 0;
      return 1;
    }
  }

  // Nothing found
  return 0;
}

/**
 *
 */
vfs_system_t *vfs_get_vfs_system (const char *tag) {
  int i;

  // Browse all registered file systems
  for (i=0; i!=VFS_MAX_FILESYSTEMS; i++) {
    // Is it taken? (ie: tag is not empty)
    if (vfs_systems[i].enabled && strcmp (tag, vfs_systems[i].info.tag) == 0) {
      return &vfs_systems[i];
    }
  }

  // Not found
  return NULL;
}

/**
 *
 */
void vfs_init (void) {
  // Clear all fs slot data
  memset (vfs_systems, 0, sizeof (vfs_systems));

  // Clear all mount tables
  memset (vfs_mount_table, 0, sizeof (vfs_mount_table));

//  vfs_create_root_node ();
}


/**
 *
 */
vfs_mount_t *vfs_get_mount_from_path (const char *path) {
  char *path_suffix;
  vfs_mount_t *mount = vsf_get_mount (path, &path_suffix);
  if (mount == NULL) return NULL;

  return mount;
}

/**
 *
 */
vfs_node_t *vfs_get_node_from_path (const char *path) {
  char component[255];
  int cs;

  kprintf ("vfs_get_node_from_path('%s')\n", path);

  // Lookup correct node or return NULL when not found
  char *path_suffix;
  vfs_mount_t *mount = vsf_get_mount (path, &path_suffix);
  if (mount == NULL) return NULL;

  kprintf ("VFS_GNFP Mount: '%s'\n", mount->mount);
  kprintf ("PATH: '%s'\n", path_suffix);

  // Start with the supernode (the only in-memory node needed)
  vfs_node_t *cur_node = mount->supernode;

  // This is a absolute path, start from root node (don't care
  char *c = path_suffix;
  if (*c != '/') return NULL;   // Must be absolute

  c++;

  /* *c points to the first directory/file AFTER the '/' and cur_node is the supernode.
   * Start browsing all directories and move to that node */

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

    // DotDot directory, go back to the parent
    if (strcmp (component, "..") == 0) {
      // @TODO: make sure parent works
      kpanic ("parent wanted...\n");
      continue;
    }

    // Find the entry if it exists
    cur_node = vfs_finddir (mount, cur_node, component);
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
int sys_mount (const char *device_path, const char *fs_type, const char *mount, const char *path, int mount_options) {
  device_t *dev_ptr;
  int i;

  // @TODO: options are not used

  // Find the filesystem itself (is it registered)
  vfs_system_t *fs_system = vfs_get_vfs_system (fs_type);
  if (! fs_system) return 0; // Cannot find registered file system



  // Check if mount is already mounted
  for (i=0; i!=VFS_MAX_MOUNTS; i++) {
    if (! vfs_mount_table[i].enabled) continue;
    if (strcmp(vfs_mount_table[i].mount, mount) == 0) return 0; // Mount already mounted
  }


  // Some entries do not have a device (cybfs?)
  if (device_path != NULL) {
    // Check device
    vfs_node_t *dev_node = vfs_get_node_from_path (device_path);
    if (! dev_node) return 0;   // Path not found

    // Cannot mount device if it's not a block device
    if ((dev_node->flags & 0x7) != FS_BLOCKDEVICE) return 0;

    dev_ptr = device_get_device (dev_node->majorNum, dev_node->minorNum);
    if (! dev_ptr) return 0;    // Cannot find the device registered to this file
  }


  // Browse mount table, find first free slot
  for (i=0; i!=VFS_MAX_MOUNTS; i++) {
    // Find free slot
    if (vfs_mount_table[i].enabled) continue;


    // Populate mount_table info
    vfs_mount_table[i].dev = dev_ptr;
    vfs_mount_table[i].system = fs_system;
    vfs_mount_table[i].system->mount_count++;      // Increase mount count for this filesystem
    strcpy (vfs_mount_table[i].mount, mount);


    // Check if mount function is available
    if (!vfs_mount_table[i].system->info.mountops || !vfs_mount_table[i].system->info.mountops->mount) return 0;

    // Do some filesystem specific mounting if needed
    vfs_mount_table[i].supernode = vfs_mount_table[i].system->info.mountops->mount (&vfs_mount_table[i], dev_ptr, path);

    // Error while doing fs specific mount init?
    if (! vfs_mount_table[i].supernode) return 0;

    // @TODO: mutex this..
    vfs_mount_table[i].enabled = 1;

    kprintf ("This system is currently mounted %d times\n", vfs_mount_table[i].system->mount_count);

    kprintf ("Mount table:\n");
    for (i=0; i!=VFS_MAX_MOUNTS; i++) {
      if (! vfs_mount_table[i].enabled) continue;
      kprintf ("%d  %-20s  %08s  %d\n", i, vfs_mount_table[i].mount, vfs_mount_table[i].system->info.tag, vfs_mount_table[i].system->mount_count);
    }

    // Return ok status
    return 1;
  }


  // Could not find a free slot
  return 0;
}


