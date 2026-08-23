#pragma once

#include <stddef.h>
#include <stdint.h>

#define FS_NAME_LENGTH 32

enum fs_node_type {
    FS_NODE_NONE = 0,
    FS_NODE_FILE,
    FS_NODE_DIRECTORY,
    FS_NODE_DEVICE
};

struct fs_node;

struct fs_operations {
    int (*read)(
        struct fs_node* node,
        uint64_t offset,
        void* buffer,
        size_t size
    );

    int (*write)(
        struct fs_node* node,
        uint64_t offset,
        const void* buffer,
        size_t size
    );

    struct fs_node* (*find)(
        struct fs_node* node,
        const char* name
    );

    int (*create)(
        struct fs_node* node,
        const char* name,
        enum fs_node_type type
    );
};

struct fs_node {
    char name[FS_NAME_LENGTH];

    enum fs_node_type type;

    uint64_t size;

    void* private_data;

    const struct fs_operations* operations;

    struct fs_node* parent;
    struct fs_node* next;
    struct fs_node* children;
};

void vfs_init(void);

int vfs_mount_root(
    struct fs_node* root
);

struct fs_node* vfs_root(void);

struct fs_node* vfs_lookup(
    const char* path
);

int vfs_create(
    const char* path,
    enum fs_node_type type
);

int vfs_read(
    const char* path,
    uint64_t offset,
    void* buffer,
    size_t size
);

int vfs_write(
    const char* path,
    uint64_t offset,
    const void* buffer,
    size_t size
);

struct fs_node* ramfs_root(void);
