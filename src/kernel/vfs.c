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
    return (node->read == NULL) ? NULL : node->read (node, offset, size, buffer);
}

Uint32 write_fs (fs_node_t *node, Uint32 offset, Uint32 size, char *buffer) {
    return (node->write == NULL) ? NULL : node->write (node, offset, size, buffer);
}

void open_fs(fs_node_t *node) {
    return (node->open == NULL) ? NULL : node->open (node);
}

void close_fs(fs_node_t *node) {
    return (node->close == NULL) ? NULL : node->close (node);
}

struct dirent *readdir_fs(fs_node_t *node, Uint32 index) {
    // Check if it's a directory
    if ((node->flags & 0x7) != FS_DIRECTORY) return NULL;
    return (node->readdir == NULL) ? NULL : node->readdir (node, index);
}

fs_node_t *finddir_fs(fs_node_t *node, char *name) {
    // Check if it's a directory
    if ((node->flags & 0x7) != FS_DIRECTORY) return NULL;
    return (node->finddir == NULL) ? NULL : node->finddir (node, name);
}

void vfs_init (void) {
  // Create VFS root hierarchy
}