/**
 * SimpleFS Implementation
 */

#include <kernel/simplefs.h>
#include <kernel/heap.h>
#include <kernel/string.h>
#include <kernel/vga.h>

// VFS operations forward declarations
static int simplefs_vfs_open(vfs_node_t *node, uint32_t flags);
static void simplefs_vfs_close(vfs_node_t *node);
static int simplefs_vfs_read(vfs_node_t *node, uint64_t offset, uint64_t size, void *buffer);
static int simplefs_vfs_write(vfs_node_t *node, uint64_t offset, uint64_t size, const void *buffer);
static vfs_node_t *simplefs_vfs_readdir(vfs_node_t *node, uint32_t index);
static vfs_node_t *simplefs_vfs_finddir(vfs_node_t *node, const char *name);

// Filesystem operations forward declarations
static int simplefs_fs_init(filesystem_t *fs, void *device);
static void simplefs_fs_destroy(filesystem_t *fs);
static vfs_node_t *simplefs_fs_get_root(filesystem_t *fs);

/**
 * Format a block device with SimpleFS
 */
int simplefs_format(block_device_t *device) {
    if (!device) {
        return -1;
    }

    vga_printf("  SimpleFS: Formatting device '%s'...\n", device->name);

    // Create superblock
    simplefs_superblock_t sb;
    memset(&sb, 0, sizeof(sb));

    sb.magic = SIMPLEFS_MAGIC;
    sb.version = SIMPLEFS_VERSION;
    sb.block_size = BLOCK_SIZE;
    sb.num_blocks = device->num_blocks;
    sb.num_inodes = SIMPLEFS_MAX_INODES;
    sb.first_inode_block = 1;
    sb.first_data_block = 1 + SIMPLEFS_INODE_BLOCKS;
    sb.free_blocks = sb.num_blocks - sb.first_data_block;
    sb.free_inodes = sb.num_inodes;

    // Write superblock to block 0
    if (device->write_block(device, 0, (uint8_t *)&sb) != 0) {
        vga_printf("  SimpleFS: Failed to write superblock\n");
        return -1;
    }

    // Initialize inode table (clear all inodes)
    simplefs_inode_t empty_inode;
    memset(&empty_inode, 0, sizeof(empty_inode));

    uint8_t inode_block[BLOCK_SIZE];
    memset(inode_block, 0, BLOCK_SIZE);

    for (uint32_t i = 0; i < SIMPLEFS_INODE_BLOCKS; i++) {
        if (device->write_block(device, sb.first_inode_block + i, inode_block) != 0) {
            vga_printf("  SimpleFS: Failed to write inode block %u\n", i);
            return -1;
        }
    }

    // Create root directory (inode 0)
    simplefs_inode_t root_inode;
    memset(&root_inode, 0, sizeof(root_inode));
    root_inode.number = 0;
    root_inode.type = SIMPLEFS_TYPE_DIR;
    root_inode.size = 0;
    root_inode.blocks = 1;
    root_inode.direct[0] = sb.first_data_block;  // Allocate first data block for root

    // Write root inode
    memcpy(inode_block, &root_inode, sizeof(root_inode));
    if (device->write_block(device, sb.first_inode_block, inode_block) != 0) {
        vga_printf("  SimpleFS: Failed to write root inode\n");
        return -1;
    }

    // Initialize root directory block (empty)
    uint8_t data_block[BLOCK_SIZE];
    memset(data_block, 0, BLOCK_SIZE);
    if (device->write_block(device, sb.first_data_block, data_block) != 0) {
        vga_printf("  SimpleFS: Failed to write root directory block\n");
        return -1;
    }

    // Update superblock with allocated resources
    sb.free_inodes--;  // Root inode
    sb.free_blocks--;  // Root directory block

    // Write updated superblock
    if (device->write_block(device, 0, (uint8_t *)&sb) != 0) {
        return -1;
    }

    vga_printf("  SimpleFS: Format complete (%u inodes, %u blocks)\n",
               sb.num_inodes, sb.num_blocks);

    return 0;
}

/**
 * Read inode from disk
 */
int simplefs_read_inode(simplefs_t *fs, uint32_t inode_num, simplefs_inode_t *inode) {
    if (!fs || !inode || inode_num >= fs->superblock.num_inodes) {
        return -1;
    }

    // Calculate which block and offset
    uint32_t inodes_per_block = BLOCK_SIZE / sizeof(simplefs_inode_t);
    uint32_t block_num = fs->superblock.first_inode_block + (inode_num / inodes_per_block);
    uint32_t offset = (inode_num % inodes_per_block) * sizeof(simplefs_inode_t);

    // Read inode block
    uint8_t block_data[BLOCK_SIZE];
    if (fs->device->read_block(fs->device, block_num, block_data) != 0) {
        return -1;
    }

    // Copy inode data
    memcpy(inode, block_data + offset, sizeof(simplefs_inode_t));
    return 0;
}

/**
 * Write inode to disk
 */
int simplefs_write_inode(simplefs_t *fs, uint32_t inode_num, const simplefs_inode_t *inode) {
    if (!fs || !inode || inode_num >= fs->superblock.num_inodes) {
        return -1;
    }

    // Calculate which block and offset
    uint32_t inodes_per_block = BLOCK_SIZE / sizeof(simplefs_inode_t);
    uint32_t block_num = fs->superblock.first_inode_block + (inode_num / inodes_per_block);
    uint32_t offset = (inode_num % inodes_per_block) * sizeof(simplefs_inode_t);

    // Read existing block
    uint8_t block_data[BLOCK_SIZE];
    if (fs->device->read_block(fs->device, block_num, block_data) != 0) {
        return -1;
    }

    // Update inode data
    memcpy(block_data + offset, inode, sizeof(simplefs_inode_t));

    // Write block back
    if (fs->device->write_block(fs->device, block_num, block_data) != 0) {
        return -1;
    }

    return 0;
}

/**
 * VFS read operation
 */
static int simplefs_vfs_read(vfs_node_t *node, uint64_t offset, uint64_t size, void *buffer) {
    if (!node || !buffer) {
        return -1;
    }

    simplefs_t *fs = (simplefs_t *)node->fs->fs_data;
    simplefs_inode_t *inode = (simplefs_inode_t *)node->fs_data;

    if (!inode || inode->type != SIMPLEFS_TYPE_FILE) {
        return -1;  // Can only read files
    }

    // Bounds check
    if (offset >= inode->size) {
        return 0;  // EOF
    }

    uint64_t to_read = size;
    if (offset + to_read > inode->size) {
        to_read = inode->size - offset;
    }

    uint64_t bytes_read = 0;
    uint8_t block_buffer[BLOCK_SIZE];

    while (bytes_read < to_read) {
        uint64_t block_index = (offset + bytes_read) / BLOCK_SIZE;
        uint64_t block_offset = (offset + bytes_read) % BLOCK_SIZE;
        uint64_t block_remaining = BLOCK_SIZE - block_offset;
        uint64_t to_copy = (to_read - bytes_read < block_remaining) ?
                           to_read - bytes_read : block_remaining;

        if (block_index >= SIMPLEFS_MAX_FILE_BLOCKS) {
            break;  // Beyond file limits
        }

        uint32_t physical_block = inode->direct[block_index];
        if (physical_block == 0) {
            break;  // Sparse file or end
        }

        // Read block
        if (fs->device->read_block(fs->device, physical_block, block_buffer) != 0) {
            return -1;
        }

        // Copy data
        memcpy((uint8_t *)buffer + bytes_read, block_buffer + block_offset, to_copy);
        bytes_read += to_copy;
    }

    return bytes_read;
}

/**
 * VFS write operation (stub for now)
 */
static int simplefs_vfs_write(vfs_node_t *node, uint64_t offset, uint64_t size, const void *buffer) {
    // TODO: Implement write functionality
    (void)node; (void)offset; (void)size; (void)buffer;
    return -1;  // Not implemented yet
}

/**
 * VFS open operation
 */
static int simplefs_vfs_open(vfs_node_t *node, uint32_t flags) {
    (void)node; (void)flags;
    return 0;  // Simple filesystems don't need complex open logic
}

/**
 * VFS close operation
 */
static void simplefs_vfs_close(vfs_node_t *node) {
    (void)node;
    // Nothing to do for simple filesystem
}

/**
 * VFS readdir operation
 */
static vfs_node_t *simplefs_vfs_readdir(vfs_node_t *node, uint32_t index) {
    if (!node) {
        return NULL;
    }

    simplefs_t *fs = (simplefs_t *)node->fs->fs_data;
    simplefs_inode_t *inode = (simplefs_inode_t *)node->fs_data;

    if (!inode || inode->type != SIMPLEFS_TYPE_DIR) {
        return NULL;  // Not a directory
    }

    // Read directory entries from first direct block
    if (inode->direct[0] == 0) {
        return NULL;  // Empty directory
    }

    uint8_t block_data[BLOCK_SIZE];
    if (fs->device->read_block(fs->device, inode->direct[0], block_data) != 0) {
        return NULL;
    }

    simplefs_direntry_t *entries = (simplefs_direntry_t *)block_data;
    uint32_t max_entries = BLOCK_SIZE / sizeof(simplefs_direntry_t);

    if (index >= max_entries) {
        return NULL;
    }

    simplefs_direntry_t *entry = &entries[index];
    if (entry->inode == 0) {
        return NULL;  // Empty entry
    }

    // Create VFS node for this entry
    vfs_node_t *child = (vfs_node_t *)kmalloc(sizeof(vfs_node_t));
    if (!child) {
        return NULL;
    }

    memset(child, 0, sizeof(vfs_node_t));
    strncpy(child->name, entry->name, sizeof(child->name) - 1);
    child->inode = entry->inode;
    child->type = entry->type;
    child->fs = node->fs;

    // Read child inode for size info
    simplefs_inode_t child_inode;
    if (simplefs_read_inode(fs, entry->inode, &child_inode) == 0) {
        child->size = child_inode.size;
    }

    child->read = simplefs_vfs_read;
    child->write = simplefs_vfs_write;
    child->open = simplefs_vfs_open;
    child->close = simplefs_vfs_close;
    child->readdir = simplefs_vfs_readdir;
    child->finddir = simplefs_vfs_finddir;

    return child;
}

/**
 * VFS finddir operation
 */
static vfs_node_t *simplefs_vfs_finddir(vfs_node_t *node, const char *name) {
    // Linear search through directory entries
    for (uint32_t i = 0; i < 100; i++) {  // Limit search
        vfs_node_t *child = simplefs_vfs_readdir(node, i);
        if (!child) {
            break;
        }

        if (strcmp(child->name, name) == 0) {
            return child;
        }

        kfree(child);
    }

    return NULL;
}

/**
 * Get root node
 */
static vfs_node_t *simplefs_fs_get_root(filesystem_t *fs) {
    if (!fs || !fs->fs_data) {
        return NULL;
    }

    simplefs_t *sfs = (simplefs_t *)fs->fs_data;

    // Read root inode (inode 0)
    simplefs_inode_t *root_inode = (simplefs_inode_t *)kmalloc(sizeof(simplefs_inode_t));
    if (!root_inode) {
        return NULL;
    }

    if (simplefs_read_inode(sfs, 0, root_inode) != 0) {
        kfree(root_inode);
        return NULL;
    }

    // Create VFS node for root
    vfs_node_t *root = (vfs_node_t *)kmalloc(sizeof(vfs_node_t));
    if (!root) {
        kfree(root_inode);
        return NULL;
    }

    memset(root, 0, sizeof(vfs_node_t));
    strcpy(root->name, "/");
    root->inode = 0;
    root->type = FILE_TYPE_DIRECTORY;
    root->size = root_inode->size;
    root->fs = fs;
    root->fs_data = root_inode;

    root->read = simplefs_vfs_read;
    root->write = simplefs_vfs_write;
    root->open = simplefs_vfs_open;
    root->close = simplefs_vfs_close;
    root->readdir = simplefs_vfs_readdir;
    root->finddir = simplefs_vfs_finddir;

    return root;
}

/**
 * Initialize filesystem
 */
static int simplefs_fs_init(filesystem_t *fs, void *device) {
    if (!fs || !device) {
        return -1;
    }

    block_device_t *bdev = (block_device_t *)device;

    // Allocate simplefs structure
    simplefs_t *sfs = (simplefs_t *)kzalloc(sizeof(simplefs_t));
    if (!sfs) {
        return -1;
    }

    sfs->device = bdev;

    // Read superblock
    uint8_t block_data[BLOCK_SIZE];
    if (bdev->read_block(bdev, 0, block_data) != 0) {
        kfree(sfs);
        return -1;
    }

    memcpy(&sfs->superblock, block_data, sizeof(simplefs_superblock_t));

    // Verify magic number
    if (sfs->superblock.magic != SIMPLEFS_MAGIC) {
        vga_printf("  SimpleFS: Invalid magic number (0x%x)\n", sfs->superblock.magic);
        kfree(sfs);
        return -1;
    }

    fs->fs_data = sfs;
    fs->device = device;

    vga_printf("  SimpleFS: Mounted successfully\n");
    return 0;
}

/**
 * Destroy filesystem
 */
static void simplefs_fs_destroy(filesystem_t *fs) {
    if (fs && fs->fs_data) {
        kfree(fs->fs_data);
        fs->fs_data = NULL;
    }
}

/**
 * Create SimpleFS filesystem driver
 */
filesystem_t *simplefs_create(block_device_t *device) {
    filesystem_t *fs = (filesystem_t *)kzalloc(sizeof(filesystem_t));
    if (!fs) {
        return NULL;
    }

    strcpy(fs->name, "simplefs");
    fs->init = simplefs_fs_init;
    fs->destroy = simplefs_fs_destroy;
    fs->get_root = simplefs_fs_get_root;
    fs->device = device;

    // Initialize the filesystem
    if (fs->init(fs, device) != 0) {
        kfree(fs);
        return NULL;
    }

    return fs;
}
