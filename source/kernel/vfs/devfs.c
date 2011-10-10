/******************************************************************************
 *
 *  File        : devfs.c
 *  Description : devfs filesystem. Simple filesystem for device files
 *
 *
 *
 *****************************************************************************/

#include "kernel.h"
#include "kmem.h"
#include "vfs.h"
#include "vfs/devfs.h"


// File operations
static struct vfs_fileops devfs_fileops = {
    .readdir = devfs_readdir,
    .finddir = devfs_finddir,
    .mknod = devfs_mknod
};

// Mount operations
static struct vfs_mountops devfs_mountops = {
    .mount = devfs_mount, .umount = devfs_umount
};

// Global structure
static vfs_info_t devfs_vfs_info = { .tag = "devfs",
                              .name = "CybOS Device Meta System",
                              .mountops = &devfs_mountops
                            };

// Root supernode
static vfs_node_t devfs_supernode = {
  .inode_nr = 0,
  .name = "/",
  .owner = 0,
  .length = 0,
  .flags = FS_DIRECTORY,
  .major_num = 0,
  .minor_num = 0,
  .fileops = &devfs_fileops,
};


// @TODO: Remove most of these items.. only /DEVICES should be present so we can mount our root system
const devfs_file_t devfs_default_layout[] = {
                                        // Dummy root
                                        { 0,  0, "devfs-root", DEVFS_TYPE_DIRECTORY, 0, 0 },
                                       };


/**
 * Intializes CybFS filesystem
 */
void devfs_init () {
  int i;

  // Clear everything
  memset (devfs_nodes, 0, sizeof (devfs_nodes));

  // Clear out all files
  for (i=0; i!=DEVFS_MAX_FILES; i++) {
    devfs_nodes[i].inode_nr = i;
    devfs_nodes[i].type = 0;
  }

  // Copy default file hierachy
  memcpy (&devfs_nodes, &devfs_default_layout, sizeof (devfs_default_layout));

  // Register filesystem to the VFS
  vfs_register_filesystem (&devfs_vfs_info);
}



/**
 * Read directory entry X (numerical dir seek)
 */
int devfs_readdir (vfs_node_t *dirnode, Uint32 index, vfs_dirent_t *target_dirent) {
  int i, found;

//  kprintf ("devfs_readdir(%s, %d)\n", dirnode->name, index);

  // We can check which 'subdirectory' we are in, by checking the parent_inode_nr of the
  // file inside the devfs system. The inode_nr of the file is the same as the index in
  // the system.
  index++;
  found = 0;
  for (i=1; i!=DEVFS_MAX_FILES; i++) {
    // This entry is empty
    if (devfs_nodes[i].type == 0) continue;

//    kprintf ("Comparing inode slot %d: %d => %d\n", i, devfs_nodes[i].parent_inode_nr, dirnode->inode_nr);

    // This this entry in the correct directory?
    if (devfs_nodes[i].parent_inode_nr == dirnode->inode_nr) {
      // An entry is found. Decrease index counter
      index--;
//      kprintf ("Found file (%s) in dir dec index to %d\n", devfs_nodes[i].name, index);
      if (index == 0) {
        found = i;
        break;
      }
    }
  }

  // No file found
  if (! found) return NULL;

//kprintf ("Populating dirent\n");
  target_dirent->inode_nr = found;
  strcpy (target_dirent->name, devfs_nodes[found].name);
//kprintf ("Done Populating dirent\n");
  return 1;
}


/**
 * Return directory entry 'name' (basically associative dir seek)
 */
int devfs_finddir (vfs_node_t *dirnode, const char *name, vfs_node_t *target_node) {
  Uint32 index, i;

//  kprintf ("devfs_finddir (%s, %s);\n", dirnode->name, name);

  // Get the directory ID where this file resides in
  index = 0;
  Uint32 parent_inode = devfs_nodes[dirnode->inode_nr].inode_nr;
  for (i=1; i!=DEVFS_MAX_FILES; i++) {
    // This entry is empty
    if (devfs_nodes[i].type == 0) continue;

    // If not in the same directory, skip
    if (devfs_nodes[i].parent_inode_nr != parent_inode) continue;

    // Is this the correct file?
    if (strcmp (devfs_nodes[i].name, name) == 0) {
      index = i;    // Save this index
      break;
    }
  }

  // Nothing found ?
  if (index == 0) return NULL;

  // Fill VFS structure with devfs information
  target_node->inode_nr = index;
  strcpy (target_node->name, devfs_nodes[index].name);
  target_node->owner = 0;
  target_node->length = 0;
  target_node->major_num = devfs_nodes[index].major_num;
  target_node->minor_num = devfs_nodes[index].minor_num;
  switch (devfs_nodes[index].type) {
    case DEVFS_TYPE_DIRECTORY :
                            target_node->flags = FS_DIRECTORY;
                            break;
    case DEVFS_TYPE_BLOCK_DEV :
                            target_node->flags = FS_BLOCKDEVICE;
                            break;
    case DEVFS_TYPE_CHAR_DEV :
                            target_node->flags = FS_CHARDEVICE;
                            break;
  }

  // Copy the mount information from the parent
  target_node->fileops = &devfs_fileops;   // Everything here is handled by CybFS of course

  // Return status
  return 1;
}

/**
 *
 */
vfs_node_t *devfs_mount (struct vfs_mount *mount, device_t *dev, const char *path) {
//  kprintf ("\n*** Mounting DEVFS_mount on %s (chroot from '%s') \n", mount->mount, path);

  // Return root node for this system
  if (strcmp (path, "/") == 0) {
//    kprintf ("DevFS: Returning root node.\n");
    return &devfs_supernode;
  }

  // Return directory node
  // @TODO: Return another node when we are not mounted to the root
//  kprintf ("DevFS: Returning directory node.\n");
  return &devfs_supernode;
}

/**
 *
 */
void devfs_umount (struct vfs_mount *mount) {
  kprintf ("\n*** Unmounting DEVFS_mount from %s\n", mount->mount);
}

/**
 *
 */
void devfs_mknod (vfs_node_t *node, const char *name, char device_type, Uint8 major_node, Uint8 minor_node) {
  int i;

//  kprintf ("\n\ndevfs_mknod(%s, %d %d, %d)\n", name, device_type, major_node, minor_node);

  // Must be a directory
  if ( (node->flags & DEVFS_TYPE_DIRECTORY) != DEVFS_TYPE_DIRECTORY) return;

  // Browse all slots for devfs
  for (i=0; i!=DEVFS_MAX_FILES; i++) {
    // Already taken
    if (devfs_nodes[i].type != 0) continue;

    devfs_nodes[i].inode_nr = i;    // Inode nr is the slot nr
    devfs_nodes[i].parent_inode_nr = node->inode_nr;    // Inode nr is the dir's nr
    strcpy (devfs_nodes[i].name, name);
    devfs_nodes[i].major_num = major_node;
    devfs_nodes[i].minor_num = minor_node;

    if (device_type == FS_BLOCKDEVICE) devfs_nodes[i].type = DEVFS_TYPE_BLOCK_DEV;
    if (device_type == FS_CHARDEVICE) devfs_nodes[i].type = DEVFS_TYPE_CHAR_DEV;
    return;
  }
}


