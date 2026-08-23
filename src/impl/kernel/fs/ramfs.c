#include "fs.h"

#include "heap.h"

static struct fs_node* root;

static int string_equals(
    const char* a,
    const char* b
)
{
    size_t i = 0;

    while (a[i] != '\0' &&
           b[i] != '\0') {

        if (a[i] != b[i]) {
            return 0;
        }

        i++;
    }

    return a[i] == '\0' &&
           b[i] == '\0';
}

static struct fs_node* ramfs_find(
    struct fs_node* node,
    const char* name
)
{
    if (node == 0) {
        return 0;
    }

    struct fs_node* child =
        node->children;

    while (child != 0) {
        if (string_equals(
                child->name,
                name
            )) {

            return child;
        }

        child = child->next;
    }

    return 0;
}

static int ramfs_read(
    struct fs_node* node,
    uint64_t offset,
    void* buffer,
    size_t size
)
{
    if (node == 0 ||
        buffer == 0 ||
        node->type != FS_NODE_FILE) {

        return -1;
    }

    if (offset >= node->size) {
        return 0;
    }

    size_t available =
        node->size - offset;

    if (size > available) {
        size = available;
    }

    uint8_t* source =
        (uint8_t*)node->private_data;

    uint8_t* destination =
        (uint8_t*)buffer;

    for (size_t i = 0;
         i < size;
         i++) {

        destination[i] =
            source[offset + i];
    }

    return (int)size;
}

static int ramfs_write(
    struct fs_node* node,
    uint64_t offset,
    const void* buffer,
    size_t size
)
{
    if (node == 0 ||
        buffer == 0 ||
        node->type != FS_NODE_FILE) {

        return -1;
    }

    if (offset + size > node->size) {
        size =
            node->size > offset
                ? node->size - offset
                : 0;
    }

    uint8_t* destination =
        (uint8_t*)node->private_data;

    const uint8_t* source =
        (const uint8_t*)buffer;

    for (size_t i = 0;
         i < size;
         i++) {

        destination[offset + i] =
            source[i];
    }

    return (int)size;
}

static struct fs_node* ramfs_create_node(
    const char* name,
    enum fs_node_type type
)
{
    struct fs_node* node =
        (struct fs_node*)kcalloc(
            1,
            sizeof(struct fs_node)
        );

    if (node == 0) {
        return 0;
    }

    for (size_t i = 0;
         i < FS_NAME_LENGTH - 1 &&
         name[i] != '\0';
         i++) {

        node->name[i] = name[i];
    }

    node->type = type;

    return node;
}

static int ramfs_create(
    struct fs_node* node,
    const char* name,
    enum fs_node_type type
)
{
    if (node == 0 ||
        node->type != FS_NODE_DIRECTORY) {

        return -1;
    }

    if (ramfs_find(node, name) != 0) {
        return -2;
    }

    struct fs_node* child =
        ramfs_create_node(
            name,
            type
        );

    if (child == 0) {
        return -3;
    }

    child->parent = node;

    child->next = node->children;

    node->children = child;

    if (type == FS_NODE_FILE) {
        child->size = 4096;

        child->private_data =
            kcalloc(
                1,
                child->size
            );

        if (child->private_data == 0) {
            return -4;
        }
    }

    return 0;
}

static const struct fs_operations ramfs_operations = {
    .read = ramfs_read,
    .write = ramfs_write,
    .find = ramfs_find,
    .create = ramfs_create
};

struct fs_node* ramfs_root(void)
{
    if (root != 0) {
        return root;
    }

    root =
        ramfs_create_node(
            "/",
            FS_NODE_DIRECTORY
        );

    if (root == 0) {
        return 0;
    }

    root->operations =
        &ramfs_operations;

    return root;
}
