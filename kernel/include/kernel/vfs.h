/**
 * Virtual Filesystem (VFS) Layer
 *
 * Provides unified interface for different filesystem implementations.
 */

#ifndef KERNEL_VFS_H
#define KERNEL_VFS_H

#include <stdint.h>
#include <stddef.h>

// Maximum number of open files per process
#define MAX_OPEN_FILES 32

// Maximum number of mounted filesystems
#define MAX_MOUNTS 8

// File types
#define FILE_TYPE_REGULAR   0x01
#define FILE_TYPE_DIRECTORY 0x02
#define FILE_TYPE_DEVICE    0x04
#define FILE_TYPE_SYMLINK   0x08

// File flags
#define O_RDONLY    0x0000
#define O_WRONLY    0x0001
#define O_RDWR      0x0002
#define O_CREAT     0x0100
#define O_TRUNC     0x0200
#define O_APPEND    0x0400

// Seek modes
#define SEEK_SET    0
#define SEEK_CUR    1
#define SEEK_END    2

// Forward declarations
struct vfs_node;
struct filesystem;
struct file_descriptor;

/**
 * VFS Node (Inode) - represents a file or directory
 */
typedef struct vfs_node {
    char name[256];              // File name
    uint32_t inode;              // Inode number
    uint32_t type;               // File type (regular, directory, etc.)
    uint32_t size;               // File size in bytes
    uint32_t permissions;        // Access permissions
    uint32_t uid;                // User ID
    uint32_t gid;                // Group ID
    uint64_t created;            // Creation time
    uint64_t modified;           // Modification time
    uint64_t accessed;           // Last access time

    struct filesystem *fs;       // Filesystem this node belongs to
    void *fs_data;               // Filesystem-specific data

    // Function pointers for operations
    int (*read)(struct vfs_node *node, uint64_t offset, uint64_t size, void *buffer);
    int (*write)(struct vfs_node *node, uint64_t offset, uint64_t size, const void *buffer);
    int (*open)(struct vfs_node *node, uint32_t flags);
    void (*close)(struct vfs_node *node);
    struct vfs_node *(*readdir)(struct vfs_node *node, uint32_t index);
    struct vfs_node *(*finddir)(struct vfs_node *node, const char *name);
} vfs_node_t;

/**
 * Filesystem driver interface
 */
typedef struct filesystem {
    char name[32];               // Filesystem type name (e.g., "simplefs", "ext2")
    uint32_t id;                 // Unique filesystem ID

    // Initialization and cleanup
    int (*init)(struct filesystem *fs, void *device);
    void (*destroy)(struct filesystem *fs);

    // Node operations
    vfs_node_t *(*get_root)(struct filesystem *fs);
    vfs_node_t *(*create_file)(struct filesystem *fs, const char *path, uint32_t permissions);
    vfs_node_t *(*create_dir)(struct filesystem *fs, const char *path, uint32_t permissions);
    int (*delete)(struct filesystem *fs, const char *path);

    void *device;                // Device (e.g., disk) this filesystem is on
    void *fs_data;               // Filesystem-specific data
} filesystem_t;

/**
 * File descriptor - represents an open file
 */
typedef struct file_descriptor {
    vfs_node_t *node;            // VFS node
    uint64_t offset;             // Current read/write offset
    uint32_t flags;              // Open flags (O_RDONLY, etc.)
    uint32_t ref_count;          // Reference count
    int in_use;                  // Is this FD in use?
} file_descriptor_t;

/**
 * Mount point
 */
typedef struct mount {
    char path[256];              // Mount path (e.g., "/", "/mnt/disk")
    filesystem_t *fs;            // Mounted filesystem
    vfs_node_t *root;            // Root node of mounted filesystem
    int in_use;                  // Is this mount active?
} mount_t;

/**
 * Directory entry
 */
typedef struct dirent {
    uint32_t inode;              // Inode number
    char name[256];              // File name
    uint32_t type;               // File type
} dirent_t;

// VFS initialization
void vfs_init(void);

// File operations
int vfs_open(const char *path, uint32_t flags);
int vfs_close(int fd);
int vfs_read(int fd, void *buffer, size_t size);
int vfs_write(int fd, const void *buffer, size_t size);
int vfs_seek(int fd, int64_t offset, int whence);
int vfs_stat(const char *path, vfs_node_t *stat_buf);

// Directory operations
int vfs_mkdir(const char *path, uint32_t permissions);
int vfs_readdir(int fd, dirent_t *dirent, uint32_t index);

// Filesystem operations
int vfs_mount(const char *path, filesystem_t *fs);
int vfs_unmount(const char *path);

// Filesystem registration
int vfs_register_filesystem(filesystem_t *fs);
filesystem_t *vfs_get_filesystem(const char *name);

// Path resolution
vfs_node_t *vfs_resolve_path(const char *path);

// File descriptor management
file_descriptor_t *vfs_get_fd(int fd);
int vfs_alloc_fd(vfs_node_t *node, uint32_t flags);
void vfs_free_fd(int fd);

#endif // KERNEL_VFS_H
