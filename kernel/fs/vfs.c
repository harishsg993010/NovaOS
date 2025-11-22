/**
 * Virtual Filesystem (VFS) Implementation
 */

#include <kernel/vfs.h>
#include <kernel/heap.h>
#include <kernel/string.h>
#include <kernel/vga.h>

// Global file descriptor table (per-process in future)
static file_descriptor_t file_descriptors[MAX_OPEN_FILES];

// Mount table
static mount_t mounts[MAX_MOUNTS];

// Registered filesystems
static filesystem_t *registered_fs[MAX_MOUNTS];
static uint32_t num_registered_fs = 0;

// Root filesystem
static vfs_node_t *vfs_root = NULL;

/**
 * Initialize VFS layer
 */
void vfs_init(void) {
    // Clear file descriptor table
    memset(file_descriptors, 0, sizeof(file_descriptors));

    // Clear mount table
    memset(mounts, 0, sizeof(mounts));

    // Clear registered filesystems
    memset(registered_fs, 0, sizeof(registered_fs));
    num_registered_fs = 0;

    vfs_root = NULL;

    vga_printf("  VFS: Initialized\n");
}

/**
 * Register a filesystem driver
 */
int vfs_register_filesystem(filesystem_t *fs) {
    if (!fs || num_registered_fs >= MAX_MOUNTS) {
        return -1;
    }

    registered_fs[num_registered_fs++] = fs;
    vga_printf("  VFS: Registered filesystem '%s'\n", fs->name);
    return 0;
}

/**
 * Get filesystem by name
 */
filesystem_t *vfs_get_filesystem(const char *name) {
    for (uint32_t i = 0; i < num_registered_fs; i++) {
        if (strcmp(registered_fs[i]->name, name) == 0) {
            return registered_fs[i];
        }
    }
    return NULL;
}

/**
 * Mount a filesystem
 */
int vfs_mount(const char *path, filesystem_t *fs) {
    if (!path || !fs) {
        return -1;
    }

    // Find free mount slot
    int slot = -1;
    for (int i = 0; i < MAX_MOUNTS; i++) {
        if (!mounts[i].in_use) {
            slot = i;
            break;
        }
    }

    if (slot == -1) {
        return -1;  // No free mount slots
    }

    // Get root node from filesystem
    vfs_node_t *root = fs->get_root(fs);
    if (!root) {
        return -1;
    }

    // Setup mount point
    strncpy(mounts[slot].path, path, sizeof(mounts[slot].path) - 1);
    mounts[slot].fs = fs;
    mounts[slot].root = root;
    mounts[slot].in_use = 1;

    // If mounting at root, set global root
    if (strcmp(path, "/") == 0) {
        vfs_root = root;
    }

    vga_printf("  VFS: Mounted '%s' at '%s'\n", fs->name, path);
    return 0;
}

/**
 * Unmount a filesystem
 */
int vfs_unmount(const char *path) {
    for (int i = 0; i < MAX_MOUNTS; i++) {
        if (mounts[i].in_use && strcmp(mounts[i].path, path) == 0) {
            mounts[i].in_use = 0;
            mounts[i].fs = NULL;
            mounts[i].root = NULL;
            return 0;
        }
    }
    return -1;
}

/**
 * Resolve path to VFS node
 * For simplicity, we only support absolute paths and simple resolution
 */
vfs_node_t *vfs_resolve_path(const char *path) {
    if (!path || path[0] != '/') {
        return NULL;  // Only absolute paths supported
    }

    // Special case: root directory
    if (strcmp(path, "/") == 0) {
        return vfs_root;
    }

    if (!vfs_root) {
        return NULL;
    }

    // Parse path components
    char path_copy[256];
    strncpy(path_copy, path + 1, sizeof(path_copy) - 1);  // Skip leading '/'

    vfs_node_t *current = vfs_root;
    char *token = path_copy;
    char *next = NULL;

    while (token && *token) {
        // Find next '/'
        next = token;
        while (*next && *next != '/') {
            next++;
        }

        // Null-terminate current component
        int has_next = (*next == '/');
        if (has_next) {
            *next = '\0';
            next++;
        } else {
            next = NULL;
        }

        // Look up component in current directory
        if (current->finddir) {
            current = current->finddir(current, token);
            if (!current) {
                return NULL;  // Component not found
            }
        } else {
            return NULL;  // Not a directory
        }

        token = next;
    }

    return current;
}

/**
 * Allocate a file descriptor
 */
int vfs_alloc_fd(vfs_node_t *node, uint32_t flags) {
    for (int i = 0; i < MAX_OPEN_FILES; i++) {
        if (!file_descriptors[i].in_use) {
            file_descriptors[i].node = node;
            file_descriptors[i].offset = 0;
            file_descriptors[i].flags = flags;
            file_descriptors[i].ref_count = 1;
            file_descriptors[i].in_use = 1;
            return i;
        }
    }
    return -1;  // No free file descriptors
}

/**
 * Get file descriptor
 */
file_descriptor_t *vfs_get_fd(int fd) {
    if (fd < 0 || fd >= MAX_OPEN_FILES || !file_descriptors[fd].in_use) {
        return NULL;
    }
    return &file_descriptors[fd];
}

/**
 * Free a file descriptor
 */
void vfs_free_fd(int fd) {
    if (fd >= 0 && fd < MAX_OPEN_FILES) {
        file_descriptors[fd].in_use = 0;
        file_descriptors[fd].node = NULL;
        file_descriptors[fd].offset = 0;
        file_descriptors[fd].flags = 0;
        file_descriptors[fd].ref_count = 0;
    }
}

/**
 * Open a file
 */
int vfs_open(const char *path, uint32_t flags) {
    // Resolve path to VFS node
    vfs_node_t *node = vfs_resolve_path(path);
    if (!node) {
        return -1;  // File not found
    }

    // Call node's open function if available
    if (node->open) {
        int result = node->open(node, flags);
        if (result != 0) {
            return -1;  // Open failed
        }
    }

    // Allocate file descriptor
    int fd = vfs_alloc_fd(node, flags);
    return fd;
}

/**
 * Close a file
 */
int vfs_close(int fd) {
    file_descriptor_t *file = vfs_get_fd(fd);
    if (!file) {
        return -1;
    }

    // Call node's close function if available
    if (file->node && file->node->close) {
        file->node->close(file->node);
    }

    // Free file descriptor
    vfs_free_fd(fd);
    return 0;
}

/**
 * Read from a file
 */
int vfs_read(int fd, void *buffer, size_t size) {
    file_descriptor_t *file = vfs_get_fd(fd);
    if (!file || !buffer) {
        return -1;
    }

    vfs_node_t *node = file->node;
    if (!node || !node->read) {
        return -1;  // No read function
    }

    // Read from current offset
    int bytes_read = node->read(node, file->offset, size, buffer);
    if (bytes_read > 0) {
        file->offset += bytes_read;
    }

    return bytes_read;
}

/**
 * Write to a file
 */
int vfs_write(int fd, const void *buffer, size_t size) {
    file_descriptor_t *file = vfs_get_fd(fd);
    if (!file || !buffer) {
        return -1;
    }

    vfs_node_t *node = file->node;
    if (!node || !node->write) {
        return -1;  // No write function
    }

    // Write from current offset
    int bytes_written = node->write(node, file->offset, size, buffer);
    if (bytes_written > 0) {
        file->offset += bytes_written;
    }

    return bytes_written;
}

/**
 * Seek in a file
 */
int vfs_seek(int fd, int64_t offset, int whence) {
    file_descriptor_t *file = vfs_get_fd(fd);
    if (!file) {
        return -1;
    }

    vfs_node_t *node = file->node;
    if (!node) {
        return -1;
    }

    uint64_t new_offset;

    switch (whence) {
        case SEEK_SET:
            new_offset = offset;
            break;
        case SEEK_CUR:
            new_offset = file->offset + offset;
            break;
        case SEEK_END:
            new_offset = node->size + offset;
            break;
        default:
            return -1;
    }

    file->offset = new_offset;
    return (int)new_offset;
}

/**
 * Get file information
 */
int vfs_stat(const char *path, vfs_node_t *stat_buf) {
    vfs_node_t *node = vfs_resolve_path(path);
    if (!node || !stat_buf) {
        return -1;
    }

    // Copy node information
    memcpy(stat_buf, node, sizeof(vfs_node_t));
    return 0;
}

/**
 * Create a directory
 */
int vfs_mkdir(const char *path, uint32_t permissions) {
    if (!path) {
        return -1;
    }

    // For simplicity, we'll delegate to the root filesystem
    if (!vfs_root || !vfs_root->fs || !vfs_root->fs->create_dir) {
        return -1;
    }

    vfs_node_t *dir = vfs_root->fs->create_dir(vfs_root->fs, path, permissions);
    return dir ? 0 : -1;
}

/**
 * Read directory entry
 */
int vfs_readdir(int fd, dirent_t *dirent, uint32_t index) {
    file_descriptor_t *file = vfs_get_fd(fd);
    if (!file || !dirent) {
        return -1;
    }

    vfs_node_t *dir = file->node;
    if (!dir || !dir->readdir) {
        return -1;  // Not a directory
    }

    vfs_node_t *entry = dir->readdir(dir, index);
    if (!entry) {
        return -1;  // No more entries
    }

    // Fill in dirent structure
    dirent->inode = entry->inode;
    strncpy(dirent->name, entry->name, sizeof(dirent->name) - 1);
    dirent->type = entry->type;

    return 0;
}
