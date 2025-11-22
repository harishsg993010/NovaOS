/**
 * ATA (IDE) Disk Driver
 *
 * Supports PIO mode for simple disk I/O
 */

#ifndef KERNEL_ATA_H
#define KERNEL_ATA_H

#include <stdint.h>
#include <kernel/block.h>

// ATA I/O ports (Primary bus)
#define ATA_PRIMARY_IO          0x1F0
#define ATA_PRIMARY_CONTROL     0x3F6
#define ATA_PRIMARY_IRQ         14

// ATA I/O ports (Secondary bus)
#define ATA_SECONDARY_IO        0x170
#define ATA_SECONDARY_CONTROL   0x376
#define ATA_SECONDARY_IRQ       15

// ATA registers (offset from base I/O port)
#define ATA_REG_DATA            0x00
#define ATA_REG_ERROR           0x01
#define ATA_REG_FEATURES        0x01
#define ATA_REG_SECTOR_COUNT    0x02
#define ATA_REG_LBA_LOW         0x03
#define ATA_REG_LBA_MID         0x04
#define ATA_REG_LBA_HIGH        0x05
#define ATA_REG_DRIVE_SELECT    0x06
#define ATA_REG_STATUS          0x07
#define ATA_REG_COMMAND         0x07

// ATA control register
#define ATA_REG_CONTROL         0x00
#define ATA_REG_ALT_STATUS      0x00

// ATA status flags
#define ATA_STATUS_ERR          0x01
#define ATA_STATUS_IDX          0x02
#define ATA_STATUS_CORR         0x04
#define ATA_STATUS_DRQ          0x08
#define ATA_STATUS_SRV          0x10
#define ATA_STATUS_DF           0x20
#define ATA_STATUS_RDY          0x40
#define ATA_STATUS_BSY          0x80

// ATA commands
#define ATA_CMD_READ_PIO        0x20
#define ATA_CMD_READ_PIO_EXT    0x24
#define ATA_CMD_WRITE_PIO       0x30
#define ATA_CMD_WRITE_PIO_EXT   0x34
#define ATA_CMD_CACHE_FLUSH     0xE7
#define ATA_CMD_IDENTIFY        0xEC

// ATA drive types
#define ATA_DRIVE_MASTER        0
#define ATA_DRIVE_SLAVE         1

/**
 * ATA device structure
 */
typedef struct ata_device {
    uint16_t base_io;            // Base I/O port
    uint16_t control;            // Control port
    uint8_t drive;               // Master (0) or Slave (1)
    uint8_t exists;              // Does this drive exist?

    uint32_t signature;          // Drive signature
    uint32_t capabilities;       // Drive capabilities
    uint32_t command_sets;       // Command sets supported
    uint64_t size;               // Size in sectors

    char model[41];              // Model string
    char serial[21];             // Serial number
    char firmware[9];            // Firmware version

    block_device_t block_dev;    // Block device interface
} ata_device_t;

// Initialize ATA subsystem
void ata_init(void);

// ATA device operations
int ata_read_sectors(ata_device_t *dev, uint64_t lba, uint32_t count, uint8_t *buffer);
int ata_write_sectors(ata_device_t *dev, uint64_t lba, uint32_t count, const uint8_t *buffer);

// Get ATA device
ata_device_t *ata_get_device(uint8_t bus, uint8_t drive);

#endif // KERNEL_ATA_H
