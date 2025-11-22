/**
 * ATA (IDE) Disk Driver Implementation
 *
 * Simple PIO mode driver for ATA disks
 */

#include <kernel/ata.h>
#include <kernel/port.h>
#include <kernel/string.h>
#include <kernel/vga.h>
#include <kernel/heap.h>

// ATA devices (primary master, primary slave, secondary master, secondary slave)
static ata_device_t ata_devices[4];

// Forward declarations
static int ata_block_read(block_device_t *dev, uint64_t block, uint8_t *buffer);
static int ata_block_write(block_device_t *dev, uint64_t block, const uint8_t *buffer);
static int ata_block_read_multi(block_device_t *dev, uint64_t start_block, uint32_t count, uint8_t *buffer);
static int ata_block_write_multi(block_device_t *dev, uint64_t start_block, uint32_t count, const uint8_t *buffer);

/**
 * Wait for ATA drive to be ready
 */
static int ata_wait_ready(uint16_t base_io, uint8_t timeout_ms) {
    uint32_t timeout = timeout_ms * 1000;  // Convert to microseconds

    while (timeout > 0) {
        uint8_t status = inb(base_io + ATA_REG_STATUS);

        if (!(status & ATA_STATUS_BSY) && (status & ATA_STATUS_RDY)) {
            return 0;  // Ready
        }

        timeout--;
        // Small delay (approximately 1us)
        for (volatile int i = 0; i < 10; i++);
    }

    return -1;  // Timeout
}

/**
 * Wait for DRQ (Data Request) flag
 */
static int ata_wait_drq(uint16_t base_io, uint8_t timeout_ms) {
    uint32_t timeout = timeout_ms * 1000;

    while (timeout > 0) {
        uint8_t status = inb(base_io + ATA_REG_STATUS);

        if (status & ATA_STATUS_ERR) {
            return -1;  // Error
        }

        if (status & ATA_STATUS_DRQ) {
            return 0;  // DRQ is set
        }

        timeout--;
        for (volatile int i = 0; i < 10; i++);
    }

    return -1;  // Timeout
}

/**
 * Identify ATA drive
 */
static int ata_identify(ata_device_t *dev) {
    uint16_t base_io = dev->base_io;
    uint8_t drive = dev->drive;

    // Select drive
    outb(base_io + ATA_REG_DRIVE_SELECT, 0xA0 | (drive << 4));

    // Wait a bit
    for (volatile int i = 0; i < 1000; i++);

    // Send IDENTIFY command
    outb(base_io + ATA_REG_COMMAND, ATA_CMD_IDENTIFY);

    // Read status
    uint8_t status = inb(base_io + ATA_REG_STATUS);
    if (status == 0) {
        return -1;  // Drive doesn't exist
    }

    // Wait for BSY to clear
    if (ata_wait_ready(base_io, 100) != 0) {
        return -1;
    }

    // Wait for DRQ
    if (ata_wait_drq(base_io, 100) != 0) {
        return -1;
    }

    // Read identification data (256 words = 512 bytes)
    uint16_t identify_data[256];
    for (int i = 0; i < 256; i++) {
        identify_data[i] = inw(base_io + ATA_REG_DATA);
    }

    // Extract information
    dev->signature = identify_data[0];
    dev->capabilities = identify_data[49];
    dev->command_sets = ((uint32_t)identify_data[83] << 16) | identify_data[82];

    // Get size (LBA28 or LBA48)
    if (identify_data[83] & (1 << 10)) {
        // LBA48
        dev->size = ((uint64_t)identify_data[103] << 48) |
                    ((uint64_t)identify_data[102] << 32) |
                    ((uint64_t)identify_data[101] << 16) |
                    identify_data[100];
    } else {
        // LBA28
        dev->size = ((uint32_t)identify_data[61] << 16) | identify_data[60];
    }

    // Extract model string (words 27-46, 40 characters)
    for (int i = 0; i < 20; i++) {
        dev->model[i * 2] = identify_data[27 + i] >> 8;
        dev->model[i * 2 + 1] = identify_data[27 + i] & 0xFF;
    }
    dev->model[40] = '\0';

    // Trim trailing spaces from model
    for (int i = 39; i >= 0; i--) {
        if (dev->model[i] == ' ') {
            dev->model[i] = '\0';
        } else {
            break;
        }
    }

    dev->exists = 1;
    return 0;
}

/**
 * Initialize ATA subsystem
 */
void ata_init(void) {
    memset(ata_devices, 0, sizeof(ata_devices));

    // Initialize primary bus devices
    ata_devices[0].base_io = ATA_PRIMARY_IO;
    ata_devices[0].control = ATA_PRIMARY_CONTROL;
    ata_devices[0].drive = ATA_DRIVE_MASTER;

    ata_devices[1].base_io = ATA_PRIMARY_IO;
    ata_devices[1].control = ATA_PRIMARY_CONTROL;
    ata_devices[1].drive = ATA_DRIVE_SLAVE;

    // Initialize secondary bus devices
    ata_devices[2].base_io = ATA_SECONDARY_IO;
    ata_devices[2].control = ATA_SECONDARY_CONTROL;
    ata_devices[2].drive = ATA_DRIVE_MASTER;

    ata_devices[3].base_io = ATA_SECONDARY_IO;
    ata_devices[3].control = ATA_SECONDARY_CONTROL;
    ata_devices[3].drive = ATA_DRIVE_SLAVE;

    // Identify all drives
    const char *bus_names[] = {"Primary", "Primary", "Secondary", "Secondary"};
    const char *drive_names[] = {"Master", "Slave", "Master", "Slave"};

    for (int i = 0; i < 4; i++) {
        if (ata_identify(&ata_devices[i]) == 0) {
            vga_printf("  ATA: %s %s - %s (%llu MB)\n",
                       bus_names[i], drive_names[i],
                       ata_devices[i].model,
                       (ata_devices[i].size * 512) / (1024 * 1024));

            // Register as block device
            ata_device_t *dev = &ata_devices[i];
            block_device_t *block_dev = &dev->block_dev;

            snprintf(block_dev->name, sizeof(block_dev->name), "hd%c", 'a' + i);
            block_dev->type = BLOCK_TYPE_DISK;
            block_dev->block_size = BLOCK_SIZE;
            block_dev->num_blocks = dev->size;
            block_dev->size = dev->size * BLOCK_SIZE;
            block_dev->read_block = ata_block_read;
            block_dev->write_block = ata_block_write;
            block_dev->read_blocks = ata_block_read_multi;
            block_dev->write_blocks = ata_block_write_multi;
            block_dev->driver_data = dev;

            block_register_device(block_dev);
        }
    }
}

/**
 * Get ATA device
 */
ata_device_t *ata_get_device(uint8_t bus, uint8_t drive) {
    int index = (bus * 2) + drive;
    if (index >= 4 || !ata_devices[index].exists) {
        return NULL;
    }
    return &ata_devices[index];
}

/**
 * Read sectors from ATA drive
 */
int ata_read_sectors(ata_device_t *dev, uint64_t lba, uint32_t count, uint8_t *buffer) {
    if (!dev || !buffer || count == 0) {
        return -1;
    }

    uint16_t base_io = dev->base_io;

    for (uint32_t i = 0; i < count; i++) {
        // Wait for drive to be ready
        if (ata_wait_ready(base_io, 100) != 0) {
            return -1;
        }

        // Select drive and set LBA mode
        outb(base_io + ATA_REG_DRIVE_SELECT, 0xE0 | (dev->drive << 4) | ((lba >> 24) & 0x0F));

        // Set sector count
        outb(base_io + ATA_REG_SECTOR_COUNT, 1);

        // Set LBA
        outb(base_io + ATA_REG_LBA_LOW, (uint8_t)lba);
        outb(base_io + ATA_REG_LBA_MID, (uint8_t)(lba >> 8));
        outb(base_io + ATA_REG_LBA_HIGH, (uint8_t)(lba >> 16));

        // Send READ command
        outb(base_io + ATA_REG_COMMAND, ATA_CMD_READ_PIO);

        // Wait for DRQ
        if (ata_wait_drq(base_io, 100) != 0) {
            return -1;
        }

        // Read sector data (256 words = 512 bytes)
        uint16_t *word_buffer = (uint16_t *)(buffer + (i * 512));
        for (int j = 0; j < 256; j++) {
            word_buffer[j] = inw(base_io + ATA_REG_DATA);
        }

        lba++;
    }

    return 0;
}

/**
 * Write sectors to ATA drive
 */
int ata_write_sectors(ata_device_t *dev, uint64_t lba, uint32_t count, const uint8_t *buffer) {
    if (!dev || !buffer || count == 0) {
        return -1;
    }

    uint16_t base_io = dev->base_io;

    for (uint32_t i = 0; i < count; i++) {
        // Wait for drive to be ready
        if (ata_wait_ready(base_io, 100) != 0) {
            return -1;
        }

        // Select drive and set LBA mode
        outb(base_io + ATA_REG_DRIVE_SELECT, 0xE0 | (dev->drive << 4) | ((lba >> 24) & 0x0F));

        // Set sector count
        outb(base_io + ATA_REG_SECTOR_COUNT, 1);

        // Set LBA
        outb(base_io + ATA_REG_LBA_LOW, (uint8_t)lba);
        outb(base_io + ATA_REG_LBA_MID, (uint8_t)(lba >> 8));
        outb(base_io + ATA_REG_LBA_HIGH, (uint8_t)(lba >> 16));

        // Send WRITE command
        outb(base_io + ATA_REG_COMMAND, ATA_CMD_WRITE_PIO);

        // Wait for DRQ
        if (ata_wait_drq(base_io, 100) != 0) {
            return -1;
        }

        // Write sector data (256 words = 512 bytes)
        const uint16_t *word_buffer = (const uint16_t *)(buffer + (i * 512));
        for (int j = 0; j < 256; j++) {
            outw(base_io + ATA_REG_DATA, word_buffer[j]);
        }

        // Flush cache
        outb(base_io + ATA_REG_COMMAND, ATA_CMD_CACHE_FLUSH);
        ata_wait_ready(base_io, 100);

        lba++;
    }

    return 0;
}

/**
 * Block device interface - read single block
 */
static int ata_block_read(block_device_t *dev, uint64_t block, uint8_t *buffer) {
    ata_device_t *ata_dev = (ata_device_t *)dev->driver_data;
    return ata_read_sectors(ata_dev, block, 1, buffer);
}

/**
 * Block device interface - write single block
 */
static int ata_block_write(block_device_t *dev, uint64_t block, const uint8_t *buffer) {
    ata_device_t *ata_dev = (ata_device_t *)dev->driver_data;
    return ata_write_sectors(ata_dev, block, 1, buffer);
}

/**
 * Block device interface - read multiple blocks
 */
static int ata_block_read_multi(block_device_t *dev, uint64_t start_block, uint32_t count, uint8_t *buffer) {
    ata_device_t *ata_dev = (ata_device_t *)dev->driver_data;
    return ata_read_sectors(ata_dev, start_block, count, buffer);
}

/**
 * Block device interface - write multiple blocks
 */
static int ata_block_write_multi(block_device_t *dev, uint64_t start_block, uint32_t count, const uint8_t *buffer) {
    ata_device_t *ata_dev = (ata_device_t *)dev->driver_data;
    return ata_write_sectors(ata_dev, start_block, count, buffer);
}
