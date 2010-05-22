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


vfs_info_t cybfs_vfs_info = { "cybfs", "CybOS File System" };


char helloworlddata[] = "Hello world!\nThis file is the first readable file from CybOS!\n\nHave fun!\n\0";


// @TODO: Remove most of these items.. only /DEVICES should be present so we can mount our root system
const cybfs_file_t default_layout[] = {
                                        { 0,  0,  "cybfs-root",      0,                    0, 0,  0, NULL },
                                        { 1,  0,  "SYSTEM",          CYBFS_TYPE_DIRECTORY, 0, 0,  0, NULL },
                                        { 2,  1,  "kernel.bin",      CYBFS_TYPE_FILE,      0, 0,  0, NULL },
                                        { 3,  1,  "bootlvl1.bin",    CYBFS_TYPE_FILE,      0, 0,  0, NULL },
                                        { 4,  1,  "bootlvl2.bin",    CYBFS_TYPE_FILE,      0, 0,  0, NULL },
                                        { 5,  1,  "fat12.drv",       CYBFS_TYPE_FILE,      0, 0,  0, NULL },
                                        { 6,  1,  "cybfs.drv",       CYBFS_TYPE_FILE,      0, 0,  0, NULL },
                                        { 7,  0,  "PROGRAMS",        CYBFS_TYPE_DIRECTORY, 0, 0,  0, NULL },
                                        { 8,  0,  "DEVICE",          CYBFS_TYPE_DIRECTORY, 0, 0,  0, NULL },
                                        { 9,  0,  "INFO",            CYBFS_TYPE_DIRECTORY, 0, 0,  0, NULL },
                                        {10,  0,  "USER",            CYBFS_TYPE_DIRECTORY, 0, 0,  0, NULL },
                                        {11, 10,  "Joshua Thijssen", CYBFS_TYPE_DIRECTORY, 0, 0,  0, NULL },
                                        {12,  9,  "cpu",             CYBFS_TYPE_FILE,      0, 0,  0, NULL },
                                        {13,  9,  "memory",          CYBFS_TYPE_FILE,      0, 0,  0, NULL },
                                        {14,  9,  "fdc",             CYBFS_TYPE_FILE,      0, 0,  0, NULL },
                                        {15,  9,  "hdc",             CYBFS_TYPE_FILE,      0, 0,  0, NULL },
                                        {16, 10,  "Bill Gates",      CYBFS_TYPE_DIRECTORY, 0, 0,  0, NULL },
                                        {17, 10,  "Linus Torvalds",  CYBFS_TYPE_DIRECTORY, 0, 0,  0, NULL },
                                        {18, 10,  "Steve Jobs",      CYBFS_TYPE_DIRECTORY, 0, 0,  0, NULL },
                                        {19, 11,  "helloworld.txt",  CYBFS_TYPE_FILE,      0, 0, 78, (char *)&helloworlddata },
                                        {20,  8,  "FLOPPY0",         CYBFS_TYPE_DEVICE,    1, 0,  0, NULL },
                                        {21,  8,  "FLOPPY1",         CYBFS_TYPE_DEVICE,    1, 1,  0, NULL },
                                        {22,  8,  "FLOPPY2",         CYBFS_TYPE_DEVICE,    1, 2,  0, NULL },
                                        {23,  8,  "FLOPPY3",         CYBFS_TYPE_DEVICE,    1, 3,  0, NULL }
                                       };

vfs_dirent_t cybfs_rd_dirent;
vfs_node_t cybfs_fd_node;

vfs_fileops_t cybfs_fops = {
    NULL,
    cybfs_read,
    cybfs_write,
    cybfs_open,
    cybfs_close,
    cybfs_readdir,
    cybfs_finddir
  };


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
//      kprintf ("Found file in dir dec index to %d\n", index);
      if (index == 0) {
        found = i;
        break;
      }
    }
  }

  // No file found
  if (! found) return NULL;

//  kprintf ("Found index: %d\n", found);

  cybfs_rd_dirent.inode_nr = index;
  strcpy (cybfs_rd_dirent.name, cybfs_nodes[found].name);

  return &cybfs_rd_dirent;
}


/**
 * Return directory entry 'name' (basically associative dir seek)
 */
vfs_node_t *cybfs_finddir (vfs_node_t *dirnode, const char *name) {
  Uint32 index, i;

//  kprintf ("cybfs_finddir (%s, %s);\n", dirnode->name, name);

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
  cybfs_fd_node.fileops = &cybfs_fops;    // This file is handled by cybfs IO operations

  // Return structure
  return &cybfs_fd_node;
}


