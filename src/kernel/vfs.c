/******************************************************************************
 *
 *  File        : vfs.c
 *  Description : Virtual file system
 *
 *****************************************************************************/

#include "kernel.h"
#include "vfs.h"

fs_node_t *fs_root = NULL;      // Root of the filesystem


Uint32 read_fs (fs_node_t *node, Uint32 offset, Uint32 size, char *buffer) {
    return (node->fileops.read == NULL) ? NULL : node->fileops.read (node, offset, size, buffer);
}

Uint32 write_fs (fs_node_t *node, Uint32 offset, Uint32 size, char *buffer) {
    return (node->fileops.write == NULL) ? NULL : node->fileops.write (node, offset, size, buffer);
}

void open_fs(fs_node_t *node) {
    return (node->fileops.open == NULL) ? NULL : node->fileops.open (node);
}

void close_fs(fs_node_t *node) {
    return (node->fileops.close == NULL) ? NULL : node->fileops.close (node);
}

struct dirent *readdir_fs(fs_node_t *node, Uint32 index) {
  kprintf ("readdir_fs (%s, %d)\n", node->name, index);
    // Check if it's a directory
    if ((node->flags & 0x7) != FS_DIRECTORY) return NULL;
    return (node->fileops.readdir == NULL) ? NULL : node->fileops.readdir (node, index);
}

fs_node_t *finddir_fs(fs_node_t *node, char *name) {
  kprintf ("finddir_fs (%s, '%s')\n", node->name, name);
    // Check if it's a directory
    if ((node->flags & 0x7) != FS_DIRECTORY) return NULL;
    return (node->fileops.finddir == NULL) ? NULL : node->fileops.finddir (node, name);
}

void vfs_init (void) {
  // Create VFS root hierarchy
}