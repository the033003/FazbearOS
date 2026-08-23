#include "fs.h"

static struct fs_node* root_node;

void vfs_init(void)
{
    root_node = 0;
}

int vfs_mount_root(
    struct fs_node* root
)
{
    if (root == 0 ||
        root->type != FS_NODE_DIRECTORY) {

        return -1;
    }

    root_node = root;

    return 0;
}

struct fs_node* vfs_root(void)
{
    return root_node;
}

static const char* skip_slashes(
    const char* path
)
{
    while (*path == '/') {
        path++;
    }

    return path;
}

struct fs_node* vfs_lookup(
    const char* path
)
{
    if (root_node == 0 ||
        path == 0) {

        return 0;
    }

    if (path[0] == '\0' ||
        (path[0] == '/' &&
         path[1] == '\0')) {

        return root_node;
    }

    struct fs_node* current =
        root_node;

    path = skip_slashes(path);

    while (*path != '\0') {
        char component[FS_NAME_LENGTH];

        size_t length = 0;

        while (*path != '\0' &&
               *path != '/' &&
               length < FS_NAME_LENGTH - 1) {

            component[length++] =
                *path++;

        }

        component[length] = '\0';

        if (current->operations == 0 ||
            current->operations->find == 0) {

            return 0;
        }

        current =
            current->operations->find(
                current,
                component
            );

        if (current == 0) {
            return 0;
        }

        path = skip_slashes(path);
    }

    return current;
}

int vfs_create(
    const char* path,
    enum fs_node_type type
)
{
    if (path == 0 ||
        root_node == 0) {

        return -1;
    }

    /*
     * This first implementation handles creation
     * directly beneath the root.
     */
    path = skip_slashes(path);

    char name[FS_NAME_LENGTH];

    size_t length = 0;

    while (path[length] != '\0' &&
           path[length] != '/' &&
           length < FS_NAME_LENGTH - 1) {

        name[length] = path[length];
        length++;
    }

    name[length] = '\0';

    if (name[0] == '\0') {
        return -2;
    }

    if (root_node->operations == 0 ||
        root_node->operations->create == 0) {

        return -3;
    }

    return root_node->operations->create(
        root_node,
        name,
        type
    );
}

int vfs_read(
    const char* path,
    uint64_t offset,
    void* buffer,
    size_t size
)
{
    struct fs_node* node =
        vfs_lookup(path);

    if (node == 0 ||
        node->operations == 0 ||
        node->operations->read == 0) {

        return -1;
    }

    return node->operations->read(
        node,
        offset,
        buffer,
        size
    );
}

int vfs_write(
    const char* path,
    uint64_t offset,
    const void* buffer,
    size_t size
)
{
    struct fs_node* node =
        vfs_lookup(path);

    if (node == 0 ||
        node->operations == 0 ||
        node->operations->write == 0) {

        return -1;
    }

    return node->operations->write(
        node,
        offset,
        buffer,
        size
    );
}
