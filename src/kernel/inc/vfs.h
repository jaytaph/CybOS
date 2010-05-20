/******************************************************************************
 *
 *  File        : vfs.h
 *  Description : Virtual File System
 *
 *****************************************************************************/
#ifndef __VFS_H__
#define __VFS_H__

    #include "ktype.h"

    // Defines for flags
    #define FS_FILE          0x01
    #define FS_DIRECTORY     0x02
    #define FS_CHARDEVICE    0x03
    #define FS_BLOCKDEVICE   0x04
    #define FS_PIPE          0x05
    #define FS_SYMLINK       0x06
    #define FS_MOUNTPOINT    0x08

    // Inode define
    typedef Uint32 inode_t;

    struct fs_node; // Forward declaration

    // All available file operations
    struct fs_file_ops {
      Uint32(*read)(struct fs_node *, Uint32, Uint32, char *);
      Uint32(*write)(struct fs_node *, Uint32, Uint32, char *);
      void (*open)(struct fs_node *);
      void (*close)(struct fs_node *);
      struct dirent * (*readdir)(struct fs_node *, Uint32);
      struct fs_node * (*finddir)(struct fs_node *, char *);
    };



    // Directory entry
    typedef struct dirent {
        char    name[128];    // Name of the directory
        inode_t inode_nr;     // Inode
    } fs_dirent_t;

    // File entry
    typedef struct fs_node {
        inode_t             inode_nr;        // Inode
        char                name[128];       // Filename
        Uint32              owner;           // Owner ID
        Uint32              length;          // File length
        Uint32              flags;           // Node type

        struct fs_file_ops  fileops;         // File operations
        struct fs_node      *ptr;            // Symlink and mount points
    } fs_node_t;


    // Root of the filesystem
    extern fs_node_t *fs_root;

    // File system functions
    Uint32 read_fs (fs_node_t *node, Uint32 offset, Uint32 size, char *buffer);
    Uint32 write_fs (fs_node_t *node, Uint32 offset, Uint32 size, char *buffer);
    void open_fs(fs_node_t *node);
    void close_fs(fs_node_t *node);
    fs_dirent_t *readdir_fs(fs_node_t *node, Uint32 index);
    fs_node_t *finddir_fs(fs_node_t *node, char *name);
    void vfs_init (void);


#endif // __VFS_H__
