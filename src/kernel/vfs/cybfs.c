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


// File operations
static struct vfs_fileops cybfs_fileops = {
    .read = cybfs_read, .write = cybfs_write,
    .open = cybfs_open, .close = cybfs_close,
    .readdir = cybfs_readdir, .finddir = cybfs_finddir
};

// Mount operations
static struct vfs_mountops cybfs_mountops = {
    .mount = cybfs_mount, .umount = cybfs_umount
};

// Global structure
static vfs_info_t cybfs_vfs_info = { .tag = "cybfs",
                              .name = "CybOS File System",
                              .mountops = &cybfs_mountops
                            };

// Root supernode
static vfs_node_t cybfs_supernode = {
  .inode_nr = 0,
  .name = "/",
  .owner = 0,
  .length = 0,
  .flags = FS_DIRECTORY,
  .majorNum = 0,
  .minorNum = 0,
  .fileops = &cybfs_fileops,
};

char helloworlddata[] = "Hello world!\nThis file is the first readable file from CybOS!\n\nHave fun!\n\0";


// @TODO: Remove most of these items.. only /DEVICES should be present so we can mount our root system
const cybfs_file_t default_layout[] = {
                                        // Dummy root
                                        { 0,  0,  "cybfs-root",      0,                    0, 0,  0, NULL },
                                        // Mandatory FS Structure
                                        { 1,  0,  "SYSTEM",          CYBFS_TYPE_DIRECTORY, 0, 0,  0, NULL },
                                        { 2,  1,  ".",               CYBFS_TYPE_DIRECTORY, 0, 0,  0, NULL },
                                        { 3,  1,  "..",              CYBFS_TYPE_DIRECTORY, 0, 0,  0, NULL },
                                        { 4,  0,  "PROGRAM",         CYBFS_TYPE_DIRECTORY, 0, 0,  0, NULL },
                                        { 5,  4,  ".",               CYBFS_TYPE_DIRECTORY, 0, 0,  0, NULL },
                                        { 6,  4,  "..",              CYBFS_TYPE_DIRECTORY, 0, 0,  0, NULL },
                                        { 7,  0,  "DEVICE",          CYBFS_TYPE_DIRECTORY, 0, 0,  0, NULL },
                                        { 8,  7,  ".",               CYBFS_TYPE_DIRECTORY, 0, 0,  0, NULL },
                                        { 9,  7,  "..",              CYBFS_TYPE_DIRECTORY, 0, 0,  0, NULL },
                                        {10,  0,  "INFO",            CYBFS_TYPE_DIRECTORY, 0, 0,  0, NULL },
                                        {11, 10,  ".",               CYBFS_TYPE_DIRECTORY, 0, 0,  0, NULL },
                                        {12, 10,  "..",              CYBFS_TYPE_DIRECTORY, 0, 0,  0, NULL },
                                        {13,  0,  "USER",            CYBFS_TYPE_DIRECTORY, 0, 0,  0, NULL },
                                        {14, 13,  ".",               CYBFS_TYPE_DIRECTORY, 0, 0,  0, NULL },
                                        {15, 13,  "..",              CYBFS_TYPE_DIRECTORY, 0, 0,  0, NULL },
                                        // Files in /SYSTEM
                                        {16,  1,  "kernel.bin",      CYBFS_TYPE_FILE,      0, 0,  0, NULL },
                                        {17,  1,  "bootlvl1.bin",    CYBFS_TYPE_FILE,      0, 0,  0, NULL },
                                        {18,  1,  "bootlvl2.bin",    CYBFS_TYPE_FILE,      0, 0,  0, NULL },
                                        {19,  1,  "fat12.drv",       CYBFS_TYPE_FILE,      0, 0,  0, NULL },
                                        {20,  1,  "cybfs.drv",       CYBFS_TYPE_FILE,      0, 0,  0, NULL },
                                        // Files in /USER
                                        {21, 13,  "Joshua Thijssen", CYBFS_TYPE_DIRECTORY, 0, 0,  0, NULL },
                                        {22, 21,  ".",               CYBFS_TYPE_DIRECTORY, 0, 0,  0, NULL },
                                        {23, 21,  "..",              CYBFS_TYPE_DIRECTORY, 0, 0,  0, NULL },
                                        {24, 21,  "helloworld.txt",  CYBFS_TYPE_FILE,      0, 0, 78, (char *)&helloworlddata },
                                        {25, 13,  "Bill Gates",      CYBFS_TYPE_DIRECTORY, 0, 0,  0, NULL },
                                        {26, 25,  ".",               CYBFS_TYPE_DIRECTORY, 0, 0,  0, NULL },
                                        {27, 25,  "..",              CYBFS_TYPE_DIRECTORY, 0, 0,  0, NULL },
                                        {28, 13,  "Linus Torvalds",  CYBFS_TYPE_DIRECTORY, 0, 0,  0, NULL },
                                        {29, 28,  ".",               CYBFS_TYPE_DIRECTORY, 0, 0,  0, NULL },
                                        {30, 28,  "..",              CYBFS_TYPE_DIRECTORY, 0, 0,  0, NULL },
                                        {31, 13,  "Steve Jobs",      CYBFS_TYPE_DIRECTORY, 0, 0,  0, NULL },
                                        {32, 32,  ".",               CYBFS_TYPE_DIRECTORY, 0, 0,  0, NULL },
                                        {33, 32,  "..",              CYBFS_TYPE_DIRECTORY, 0, 0,  0, NULL },
                                        // Files in /INFO
                                        {34, 10,  "cpu",             CYBFS_TYPE_FILE,      0, 0,  0, NULL },
                                        {35, 10,  "memory",          CYBFS_TYPE_FILE,      0, 0,  0, NULL },
                                        {36, 10,  "fdc",             CYBFS_TYPE_FILE,      0, 0,  0, NULL },
                                        {37, 10,  "hdc",             CYBFS_TYPE_FILE,      0, 0,  0, NULL },
                                        // Files in /DEVICE
                                        {38,  7,  "FLOPPY0",         CYBFS_TYPE_BLOCK_DEV, 1, 0,  0, NULL },
                                        {39,  7,  "FLOPPY1",         CYBFS_TYPE_BLOCK_DEV, 1, 1,  0, NULL },
                                        {40,  7,  "FLOPPY2",         CYBFS_TYPE_BLOCK_DEV, 1, 2,  0, NULL },
                                        {41,  7,  "FLOPPY3",         CYBFS_TYPE_BLOCK_DEV, 1, 3,  0, NULL }
                                       };

vfs_dirent_t cybfs_rd_dirent;
vfs_node_t cybfs_fd_node;


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
Uint32 cybfs_read (struct vfs_mount *mount, vfs_node_t *node, Uint32 offset, Uint32 size, char *buffer) {
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
Uint32 cybfs_write (struct vfs_mount *mount, vfs_node_t *node, Uint32 offset, Uint32 size, char *buffer) {
  kprintf ("cybfs_write()\n");
  // Cannot write at the moment
  return 0;
}

/**
 * Opens a file
 */
void cybfs_open (struct vfs_mount *mount, vfs_node_t *node) {
  kprintf ("cybfs_open()\n");
  // Cannot open files
}

/**
 * Closes a file
 */
void cybfs_close (struct vfs_mount *mount, vfs_node_t *node) {
  kprintf ("cybfs_close()\n");
  // Cannot close files
}

/**
 * Read directory entry X (numerical dir seek)
 */
vfs_dirent_t *cybfs_readdir (struct vfs_mount *mount, vfs_node_t *dirnode, Uint32 index) {
  int i, found;

//  kprintf ("cybfs_readdir(%s, %d)\n", dirnode->name, index);

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
vfs_node_t *cybfs_finddir (struct vfs_mount *mount, vfs_node_t *dirnode, const char *name) {
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
  cybfs_fd_node.majorNum = cybfs_fd_node.minorNum = 0;
  cybfs_fd_node.majorNum = cybfs_nodes[index].device_major_num;
  cybfs_fd_node.minorNum = cybfs_nodes[index].device_minor_num;
  switch (cybfs_nodes[index].type) {
    case CYBFS_TYPE_FILE :
                            cybfs_fd_node.flags = FS_FILE;
                            break;
    case CYBFS_TYPE_DIRECTORY :
                            cybfs_fd_node.flags = FS_DIRECTORY;
                            break;
    case CYBFS_TYPE_BLOCK_DEV :
                            cybfs_fd_node.flags = FS_BLOCKDEVICE;
                            break;
    case CYBFS_TYPE_CHAR_DEV :
                            cybfs_fd_node.flags = FS_CHARDEVICE;
                            break;
  }

  // Copy the mount information from the parent
  cybfs_fd_node.fileops = &cybfs_fileops;   // Everything here is handled by CybFS of course

  // Return structure
  return &cybfs_fd_node;
}

/**
 *
 */
vfs_node_t *cybfs_mount (struct vfs_mount *mount, device_t *dev, const char *path) {
  kprintf ("\n*** Mounting CYBFS_mount on %s (chroot from '%s') \n", mount->mount, path);

  // Return root node for this system
  if (strcmp (path, "/") == 0) {
    kprintf ("CybFS: Returning root node.\n");
    return &cybfs_supernode;
  }

  // Return directory node
  // @TODO: Return another node when we are not mounted to the root
  kprintf ("CybFS: Returning directory node.\n");
  return &cybfs_supernode;
}

/**
 *
 */
void cybfs_umount (struct vfs_mount *mount) {
  kprintf ("\n*** Unmounting CYBFS_mount from %s\n", mount->mount);
}


