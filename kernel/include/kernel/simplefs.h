/**
 * SimpleFS - Simple Filesystem Implementation
 *
 * A basic filesystem with inodes and data blocks.
 * Layout:
 * - Block 0: Superblock
 * - Block 1-N: Inode table
 * - Block N+1-M: Data blocks
 */

#ifndef KERNEL_SIMPLEFS_H
#define KERNEL_SIMPLEFS_H

#include <stdint.h>
#include <kernel/vfs.h>
#include <kernel/block.h>

#define SIMPLEFS_MAGIC 0x53494D50   // "SIMP"
#define SIMPLEFS_VERSION 1

#define SIMPLEFS_MAX_FILENAME 56
#define SIMPLEFS_MAX_INODES 256
#define SIMPLEFS_INODE_BLOCKS 2      // 2 blocks for inode table
#define SIMPLEFS_MAX_FILE_BLOCKS 12  // Direct blocks per inode

// File types
#define SIMPLEFS_TYPE_FILE      1
#define SIMPLEFS_TYPE_DIR       2

/**
 * Superblock (512 bytes, fits in one block)
 */
typedef struct simplefs_superblock {
    uint32_t magic;                  // Magic number (0x53494D50)
    uint32_t version;                // Filesystem version
    uint32_t block_size;             // Block size (usually 512)
    uint32_t num_blocks;             // Total number of blocks
    uint32_t num_inodes;             // Total number of inodes
    uint32_t first_inode_block;      // First block of inode table
    uint32_t first_data_block;       // First data block
    uint32_t free_blocks;            // Number of free blocks
    uint32_t free_inodes;            // Number of free inodes
    uint8_t  reserved[476];          // Reserved for future use
} __attribute__((packed)) simplefs_superblock_t;

/**
 * Inode (64 bytes)
 */
typedef struct simplefs_inode {
    uint32_t number;                 // Inode number
    uint32_t type;                   // File type (file, directory)
    uint32_t size;                   // File size in bytes
    uint32_t blocks;                 // Number of blocks used
    uint32_t direct[SIMPLEFS_MAX_FILE_BLOCKS];  // Direct block pointers
    uint32_t created;                // Creation time
    uint32_t modified;               // Modification time
} __attribute__((packed)) simplefs_inode_t;

/**
 * Directory entry (64 bytes)
 */
typedef struct simplefs_direntry {
    uint32_t inode;                  // Inode number (0 = unused)
    char name[SIMPLEFS_MAX_FILENAME];  // File name
    uint32_t type;                   // File type
} __attribute__((packed)) simplefs_direntry_t;

/**
 * SimpleFS state
 */
typedef struct simplefs {
    block_device_t *device;          // Block device
    simplefs_superblock_t superblock;  // Cached superblock
    simplefs_inode_t *inode_cache;   // Cached inodes
    uint8_t *block_bitmap;           // Block allocation bitmap
    uint8_t *inode_bitmap;           // Inode allocation bitmap
} simplefs_t;

// Initialize SimpleFS
filesystem_t *simplefs_create(block_device_t *device);

// Format a device with SimpleFS
int simplefs_format(block_device_t *device);

// Mount SimpleFS
int simplefs_mount(block_device_t *device);

// Create file
int simplefs_create_file(simplefs_t *fs, const char *name, uint32_t parent_inode);

// Create directory
int simplefs_create_dir(simplefs_t *fs, const char *name, uint32_t parent_inode);

// Read inode
int simplefs_read_inode(simplefs_t *fs, uint32_t inode_num, simplefs_inode_t *inode);

// Write inode
int simplefs_write_inode(simplefs_t *fs, uint32_t inode_num, const simplefs_inode_t *inode);

// Allocate block
uint32_t simplefs_alloc_block(simplefs_t *fs);

// Free block
void simplefs_free_block(simplefs_t *fs, uint32_t block_num);

// Allocate inode
uint32_t simplefs_alloc_inode(simplefs_t *fs);

// Free inode
void simplefs_free_inode(simplefs_t *fs, uint32_t inode_num);

#endif // KERNEL_SIMPLEFS_H
