/**
 * Block Device Management
 */

#include <kernel/block.h>
#include <kernel/string.h>
#include <kernel/vga.h>

#define MAX_BLOCK_DEVICES 16

static block_device_t *block_devices[MAX_BLOCK_DEVICES];
static uint32_t num_block_devices = 0;

/**
 * Initialize block device subsystem
 */
void block_init(void) {
    memset(block_devices, 0, sizeof(block_devices));
    num_block_devices = 0;
    vga_printf("  Block: Initialized\n");
}

/**
 * Register a block device
 */
int block_register_device(block_device_t *dev) {
    if (!dev || num_block_devices >= MAX_BLOCK_DEVICES) {
        return -1;
    }

    block_devices[num_block_devices++] = dev;
    vga_printf("  Block: Registered device '%s' (%llu blocks, %llu bytes)\n",
               dev->name, dev->num_blocks, dev->size);
    return 0;
}

/**
 * Get block device by name
 */
block_device_t *block_get_device(const char *name) {
    for (uint32_t i = 0; i < num_block_devices; i++) {
        if (strcmp(block_devices[i]->name, name) == 0) {
            return block_devices[i];
        }
    }
    return NULL;
}

/**
 * Read data from block device at byte offset
 */
int block_read(block_device_t *dev, uint64_t offset, uint64_t size, void *buffer) {
    if (!dev || !buffer || !dev->read_blocks) {
        return -1;
    }

    uint64_t block_start = offset / dev->block_size;
    uint64_t block_offset = offset % dev->block_size;
    uint64_t blocks_needed = (block_offset + size + dev->block_size - 1) / dev->block_size;

    // For simplicity, allocate temporary buffer for aligned read
    uint8_t *temp_buffer = (uint8_t *)buffer;

    // If not block-aligned, need temporary buffer
    if (block_offset != 0 || size % dev->block_size != 0) {
        // TODO: Implement proper buffering
        // For now, only support block-aligned reads
        if (block_offset != 0) {
            return -1;
        }
    }

    // Read blocks
    int result = dev->read_blocks(dev, block_start, blocks_needed, temp_buffer);
    if (result != 0) {
        return -1;
    }

    return size;
}

/**
 * Write data to block device at byte offset
 */
int block_write(block_device_t *dev, uint64_t offset, uint64_t size, const void *buffer) {
    if (!dev || !buffer || !dev->write_blocks) {
        return -1;
    }

    uint64_t block_start = offset / dev->block_size;
    uint64_t block_offset = offset % dev->block_size;
    uint64_t blocks_needed = (block_offset + size + dev->block_size - 1) / dev->block_size;

    // For simplicity, only support block-aligned writes for now
    if (block_offset != 0 || size % dev->block_size != 0) {
        return -1;
    }

    // Write blocks
    int result = dev->write_blocks(dev, block_start, blocks_needed, (const uint8_t *)buffer);
    if (result != 0) {
        return -1;
    }

    return size;
}
