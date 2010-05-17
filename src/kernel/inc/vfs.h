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

    // File function defines
    typedef Uint32(*read_type_t)(struct fs_node *, Uint32, Uint32, char *);
    typedef Uint32(*write_type_t)(struct fs_node *, Uint32, Uint32, char *);
    typedef void (*open_type_t)(struct fs_node *);
    typedef void (*close_type_t)(struct fs_node *);
    typedef struct dirent * (*readdir_type_t)(struct fs_node *, Uint32);
    typedef struct fs_node * (*finddir_type_t)(struct fs_node *, char *);



    // Directory entry
    typedef struct dirent {
        char    name[128];
        inode_t inode_nr;
    } fs_dirent_t;

    // File entry
    typedef struct fs_node {
        inode_t        inode_nr;        // Inode
        char           name[128];       // Filename
        Uint32         owner;           // Owner ID
        Uint32         length;          // File length
        Uint32         flags;           // Node type

        read_type_t    read;            // Function pointers
        write_type_t   write;
        open_type_t    open;
        close_type_t   close;
        readdir_type_t readdir;
        finddir_type_t finddir;

        struct fs_node  *ptr;           // Symlink and mount points
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
