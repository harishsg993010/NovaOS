/**
 * Block Device Interface
 *
 * Provides abstraction for block devices (disks, partitions, etc.)
 */

#ifndef KERNEL_BLOCK_H
#define KERNEL_BLOCK_H

#include <stdint.h>
#include <stddef.h>

// Block device types
#define BLOCK_TYPE_DISK         1
#define BLOCK_TYPE_PARTITION    2
#define BLOCK_TYPE_RAMDISK      3

// Standard block size (most disks use 512 bytes)
#define BLOCK_SIZE 512

/**
 * Block device structure
 */
typedef struct block_device {
    char name[32];               // Device name (e.g., "hda", "sda")
    uint32_t type;               // Device type
    uint32_t block_size;         // Size of one block in bytes
    uint64_t num_blocks;         // Total number of blocks
    uint64_t size;               // Total size in bytes

    // Operations
    int (*read_block)(struct block_device *dev, uint64_t block, uint8_t *buffer);
    int (*write_block)(struct block_device *dev, uint64_t block, const uint8_t *buffer);
    int (*read_blocks)(struct block_device *dev, uint64_t start_block, uint32_t count, uint8_t *buffer);
    int (*write_blocks)(struct block_device *dev, uint64_t start_block, uint32_t count, const uint8_t *buffer);

    void *driver_data;           // Driver-specific data
} block_device_t;

// Block device management
void block_init(void);
int block_register_device(block_device_t *dev);
block_device_t *block_get_device(const char *name);

// Helper functions for reading/writing
int block_read(block_device_t *dev, uint64_t offset, uint64_t size, void *buffer);
int block_write(block_device_t *dev, uint64_t offset, uint64_t size, const void *buffer);

#endif // KERNEL_BLOCK_H
