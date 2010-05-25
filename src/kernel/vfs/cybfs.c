/******************************************************************************
 *
 *  File        : cybfs.c
 *  Description : cybfs filesystem. Simple filesystem for memory
 *
 *
 *
 *****************************************************************************/

#include "kernel.h"
#include "kmem.h"
#include "vfs.h"
#include "vfs/cybfs.h"


vfs_info_t cybfs_vfs_info = { "cybfs", "CybOS File System", cybfs_mount, cybfs_umount, NULL };

// @TODO: Remove most of these items.. only /DEVICES should be present so we can mount our root system
const cybfs_file_t default_layout[] = {
                                        { 0,  0,  "cybfs-root",      0,                    0, 0,  0, NULL },
                                        { 1,  0,  "SYSTEM",          CYBFS_TYPE_DIRECTORY, 0, 0,  0, NULL },
                                        { 2,  0,  "PROGRAMS",        CYBFS_TYPE_DIRECTORY, 0, 0,  0, NULL },
                                        { 3,  0,  "DEVICE",          CYBFS_TYPE_DIRECTORY, 0, 0,  0, NULL },
                                        { 4,  0,  "INFO",            CYBFS_TYPE_DIRECTORY, 0, 0,  0, NULL },
                                        { 5,  0,  "USER",            CYBFS_TYPE_DIRECTORY, 0, 0,  0, NULL },
                                        { 6,  3,  "FLOPPY0",         CYBFS_TYPE_DEVICE,    1, 0,  0, NULL },
                                        { 7,  3,  "FLOPPY1",         CYBFS_TYPE_DEVICE,    1, 1,  0, NULL },
                                        { 8,  3,  "FLOPPY2",         CYBFS_TYPE_DEVICE,    1, 2,  0, NULL },
                                        { 9,  3,  "FLOPPY3",         CYBFS_TYPE_DEVICE,    1, 3,  0, NULL }
                                      };

vfs_dirent_t cybfs_rd_dirent;     // Fixed directory entry returned by readdir()
vfs_node_t cybfs_fd_node;         // Fixed node entry returned by finddir()

vfs_fileops_t cybfs_fileops = {   // File operations for CybFS files
    cybfs_read,
    cybfs_write,
    cybfs_open,
    cybfs_close,
    cybfs_readdir,
    cybfs_finddir
};


Uint32 (*mount)(struct vfs_node *, device_t *) {
  kpanic ("Cannot mount a CybFS file system (yet).");
  return 0;
}

Uint32 (*umount)(struct vfs_node) {
  // We need nothing to do...
  return 1;
}


/**
 * Intializes CybFS filesystem
 */
void cybfs_init () {
  int i;

  // Clear out all files
  for (i=0; i!=CYBFS_MAX_FILES; i++) {
    cybfs_nodes[i].inode_nr = i;
    cybfs_nodes[i].type = 0;
  }

  // Copy default file hierachy
  memcpy (&cybfs_nodes, &default_layout, sizeof (default_layout));

  // Register filesystem to the VFS
  vfs_register_filesystem (&cybfs_vfs_info);
}


/**
 * Read file data
 */
Uint32 cybfs_read (vfs_node_t *node, Uint32 offset, Uint32 size, char *buffer) {
  kprintf ("cybfs_read(%d, %d, %d, %08X)\n", node->inode_nr, offset, size, buffer);

  // Offset is outside file
  if (offset > cybfs_nodes[node->inode_nr].length) return 0;

  // Don't read more than we are capable off
  if (offset + size > cybfs_nodes[node->inode_nr].length) size = cybfs_nodes[node->inode_nr].length - offset;

  // Copy file data to buffer
  memcpy (buffer, cybfs_nodes[node->inode_nr].data+offset, size);

  // Return number of bytes read
  return size;
}


/**
 * Writes data to file
 */
Uint32 cybfs_write (vfs_node_t *node, Uint32 offset, Uint32 size, char *buffer) {
  kprintf ("cybfs_write()\n");
  // Cannot write at the moment
  return 0;
}


/**
 * Opens a file
 */
void cybfs_open (vfs_node_t *node) {
  kprintf ("cybfs_open()\n");
  // Cannot open files
}


/**
 * Closes a file
 */
void cybfs_close (vfs_node_t *node) {
  kprintf ("cybfs_close()\n");
  // Cannot close files
}


/**
 * Read directory entry X (numerical dir seek)
 */
vfs_dirent_t *cybfs_readdir (vfs_node_t *dirnode, Uint32 index) {
  int i, found;

  // We can check which 'subdirectory' we are in, by checking the parent_inode_nr of the
  // file inside the cybfs system. The inode_nr of the file is the same as the index in
  // the system.
  index++;
  found = 0;
  for (i=1; i!=CYBFS_MAX_FILES; i++) {
    // This entry is empty
    if (cybfs_nodes[i].type == 0) continue;

    // This this entry in the correct directory?
    if (cybfs_nodes[i].parent_inode_nr == dirnode->inode_nr) {
      // An entry is found. Decrease index counter
      index--;

      // Found the index'th item?
      if (index == 0) {
        found = i;
        break;
      }
    }
  }

  // No file found
  if (! found) return NULL;

  // Return directory entry
  cybfs_rd_dirent.inode_nr = index;   // Inode is the index
  strcpy (cybfs_rd_dirent.name, cybfs_nodes[found].name);

  return &cybfs_rd_dirent;
}


/**
 * Returns directory entry 'name' (basically associative dir seek)
 */
vfs_node_t *cybfs_finddir (vfs_node_t *dirnode, const char *name) {
  Uint32 index, i;

  // Get the directory ID where this file resides in
  index = 0;
  Uint32 parent_inode = cybfs_nodes[dirnode->inode_nr].inode_nr;
  for (i=1; i!=CYBFS_MAX_FILES; i++) {
    // This entry is empty
    if (cybfs_nodes[i].type == 0) continue;

    // If not in the same directory, skip
    if (cybfs_nodes[i].parent_inode_nr != parent_inode) continue;

    // Is this the correct file?
    if (strcmp (cybfs_nodes[i].name, name) == 0) {
      index = i;    // Save this index
      break;
    }
  }

  // Nothing found ?
  if (index == 0) return NULL;

  // Fill VFS structure with cybfs information
  cybfs_fd_node.inode_nr = index;
  strcpy (cybfs_fd_node.name, cybfs_nodes[index].name);
  cybfs_fd_node.owner = 0;
  cybfs_fd_node.length = cybfs_nodes[index].length;
  cybfs_fd_node.flags = ((cybfs_nodes[index].type & CYBFS_TYPE_FILE) == CYBFS_TYPE_FILE) ? FS_FILE : FS_DIRECTORY;
  cybfs_fd_node.majorNum = cybfs_fd_node.minorNum = 0;
  cybfs_fd_node.fileops = &cybfs_fileops;    // This file is handled by cybfs IO operations

  // Return structure
  return &cybfs_fd_node;
}


