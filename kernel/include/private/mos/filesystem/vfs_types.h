// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "lib/structures/list.h"
#include "lib/structures/tree.h"
#include "lib/sync/mutex.h"
#include "mos/filesystem/fs_types.h"
#include "mos/io/io.h"
#include "mos/platform/platform.h"
#include "mos/types.h"

typedef struct _dentry dentry_t;
typedef struct _inode inode_t;
typedef struct _mount mount_t;
typedef struct _superblock superblock_t;
typedef struct _filesystem filesystem_t;
typedef struct _file file_t;

typedef u64 dev_t;

typedef struct _dir_iterator_state
{
    size_t dir_nth;
    size_t buf_capacity;
    size_t buf_written;
    char *buf;
} dir_iterator_state_t;

typedef size_t(dentry_iterator_op)(dir_iterator_state_t *state, u64 ino, const char *name, size_t name_len, file_type_t type);

typedef struct
{
    bool (*write_inode)(inode_t *, bool should_sync); // write inode to disk

    // this method is called by the VFS when an inode is marked dirty. This is specifically for the inode itself being marked dirty, not its data. If
    // the update needs to be persisted by fdatasync(), then I_DIRTY_DATASYNC will be set in the flags argument. I_DIRTY_TIME will be set in the flags
    // in case lazytime is enabled and inode_t has times updated since the last ->dirty_inode call.
    bool (*inode_dirty)(inode_t *, int flags);
    void (*release_superblock)(superblock_t *); // "called when the VFS wishes to free the superblock (i.e. unmount). This is called with the superblock lock held"
} superblock_ops_t;

typedef struct
{
    bool (*lookup)(inode_t *dir, dentry_t *dentry);                                                    // lookup a file in a directory
    bool (*newfile)(inode_t *dir, dentry_t *dentry, file_type_t type, file_perm_t perm);               // create a new file
    bool (*hardlink)(dentry_t *old_dentry, inode_t *dir, dentry_t *new_dentry);                        // create a hard link
    bool (*symlink)(inode_t *dir, dentry_t *dentry, const char *symname);                              // create a symbolic link
    bool (*unlink)(inode_t *dir, dentry_t *dentry);                                                    // remove a file
    bool (*mkdir)(inode_t *dir, dentry_t *dentry, file_perm_t perm);                                   // create a new directory
    bool (*rmdir)(inode_t *dir, dentry_t *dentry);                                                     // remove a directory
    bool (*mknode)(inode_t *dir, dentry_t *dentry, file_type_t type, file_perm_t perm, dev_t dev);     // create a new device file
    bool (*rename)(inode_t *old_dir, dentry_t *old_dentry, inode_t *new_dir, dentry_t *new_dentry);    // rename a file
    size_t (*readlink)(dentry_t *dentry, char *buffer, size_t buflen);                                 // read the contents of a symbolic link
    size_t (*iterate_dir)(inode_t *dir, dir_iterator_state_t *iterator_state, dentry_iterator_op *op); // iterate over the contents of a directory
} inode_ops_t;

typedef struct
{
    dentry_t *(*mount)(filesystem_t *fs, const char *dev_name, const char *mount_options);
    // void (*release_superblock)(superblock_t *sb);
} filesystem_ops_t;

typedef struct
{
    bool (*open)(inode_t *inode, file_t *file);
    ssize_t (*read)(file_t *file, void *buf, size_t size);
    ssize_t (*write)(file_t *file, const void *buf, size_t size);
    int (*flush)(file_t *file);
    int (*mmap)(file_t *file, void *addr, size_t size, vmblock_t *vmblock);
} file_ops_t;

typedef struct _superblock
{
    bool dirty;
    dentry_t *root;
    list_node_t mounts;
    const superblock_ops_t *ops;
} superblock_t;

typedef struct _dentry
{
    as_tree;
    spinlock_t lock;
    atomic_t refcount;
    inode_t *inode;
    const char *name;
    superblock_t *superblock; // The root of the dentry tree
    bool is_mountpoint;
    void *private; // fs-specific data
} dentry_t;

typedef struct _inode
{
    u64 ino;                    // inode number
    file_stat_t stat;           // type, permissions, uid, gid, sticky, suid, sgid, size
    file_stat_time_t times;     // accessed, created, modified
    superblock_t *superblock;   // superblock of this inode
    ssize_t nlinks;             // number of hard links to this inode
    const inode_ops_t *ops;     // operations on this inode
    const file_ops_t *file_ops; // operations on files of this inode
    void *private;              // private data
} inode_t;

typedef struct _filesystem
{
    as_linked_list;
    const filesystem_ops_t *ops;
    const char *name;
    list_node_t superblocks;
} filesystem_t;

typedef struct _mount
{
    dentry_t *root;       // root of the mounted tree
    dentry_t *mountpoint; // where the tree is mounted
    superblock_t *superblock;
    filesystem_t *fs;
} mount_t;

typedef struct _file
{
    io_t io; // refcount is tracked by the io_t
    dentry_t *dentry;
    mutex_t offset_lock; // protects the offset field
    size_t offset;       // tracks the current position in the file
} file_t;
