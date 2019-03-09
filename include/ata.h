#ifndef _ATA_H
#define _ATA_H


// refernce : http://wiki.osdev.org/IDE

#include <types.h>
#include <pci.h>

// Command/Status Port returns a bit mask referring to the status of a channel when read
// Status Registers
#define ATA_SR_BSY     0x80
#define ATA_SR_DRDY    0x40
#define ATA_SR_DF      0x20
#define ATA_SR_DSC     0x10
#define ATA_SR_DRQ     0x08
#define ATA_SR_CORR    0x04
#define ATA_SR_IDX     0x02
#define ATA_SR_ERR     0x01

// The Features/Error Port, which returns the most recent error upon read, has these possible bit masks 
// Error Registers
#define ATA_ER_BBK      0x80
#define ATA_ER_UNC      0x40
#define ATA_ER_MC       0x20
#define ATA_ER_IDNF     0x10
#define ATA_ER_MCR      0x08
#define ATA_ER_ABRT     0x04
#define ATA_ER_TK0NF    0x02
#define ATA_ER_AMNF     0x01

// When you write to the Command/Status port, you are executing one of the commands below
#define ATA_CMD_READ_PIO          0x20
#define ATA_CMD_READ_PIO_EXT      0x24
#define ATA_CMD_READ_DMA          0xC8
#define ATA_CMD_READ_DMA_EXT      0x25
#define ATA_CMD_WRITE_PIO         0x30
#define ATA_CMD_WRITE_PIO_EXT     0x34
#define ATA_CMD_WRITE_DMA         0xCA
#define ATA_CMD_WRITE_DMA_EXT     0x35
#define ATA_CMD_CACHE_FLUSH       0xE7
#define ATA_CMD_CACHE_FLUSH_EXT   0xEA
#define ATA_CMD_PACKET            0xA0
#define ATA_CMD_IDENTIFY_PACKET   0xA1
#define ATA_CMD_IDENTIFY          0xEC

// The commands for ATAPI devices
#define      ATAPI_CMD_READ       0xA8
#define      ATAPI_CMD_EJECT      0x1B


// ATA_CMD_IDENTIFY_PACKET and ATA_CMD_IDENTIFY return a buffer of 512 bytes called the identification space; the following definitions are used to read information from the identification space; each of which is the offset in the buffer
#define ATA_IDENT_DEVICETYPE   0
#define ATA_IDENT_CYLINDERS    2
#define ATA_IDENT_HEADS        6
#define ATA_IDENT_SECTORS      12
#define ATA_IDENT_SERIAL       20
#define ATA_IDENT_MODEL        54
#define ATA_IDENT_CAPABILITIES 98
#define ATA_IDENT_FIELDVALID   106
#define ATA_IDENT_MAX_LBA      120
#define ATA_IDENT_COMMANDSETS  164
#define ATA_IDENT_MAX_LBA_EXT  200


// When you select a drive, you should specify the interface type and whether it is the master or slave
#define IDE_ATA        0x00
#define IDE_ATAPI      0x01
 
#define ATA_MASTER     0x00
#define ATA_SLAVE      0x01

// Task File is a range of 8 ports which are offsets from BAR0 (primary channel) and/or BAR2 (secondary channel). 
/* To exemplify:

    * BAR0 + 0 is first port.
    * BAR0 + 1 is second port.
    * BAR0 + 2 is the third.
*/
// We can now say that each channel has 13 registers. For the primary channel, we use these values
/*
    * Data Register: BAR0 + 0; 			// Read-Write
    * Error Register: BAR0 + 1; 		// Read Only
    * Features Register: BAR0 + 1; 		// Write Only
    * SECCOUNT0: BAR0 + 2; 			// Read-Write
    * LBA0: BAR0 + 3; 				// Read-Write
    * LBA1: BAR0 + 4; 				// Read-Write
    * LBA2: BAR0 + 5; 				// Read-Write
    * HDDEVSEL: BAR0 + 6; 			// Read-Write, used to select a drive in the channel.
    * Command Register: BAR0 + 7; 		// Write Only.
    * Status Register: BAR0 + 7; 		// Read Only.
    * Alternate Status Register: BAR1 + 2; 	// Read Only.
    * Control Register: BAR1 + 2; 		// Write Only.
    * DEVADDRESS: BAR1 + 3; 			// I don't know what is the benefit from this register. 
*/
#define ATA_REG_DATA       0x00
#define ATA_REG_ERROR      0x01
#define ATA_REG_FEATURES   0x01
#define ATA_REG_SECCOUNT0  0x02
#define ATA_REG_LBA0       0x03
#define ATA_REG_LBA1       0x04
#define ATA_REG_LBA2       0x05
#define ATA_REG_HDDEVSEL   0x06
#define ATA_REG_COMMAND    0x07
#define ATA_REG_STATUS     0x07
#define ATA_REG_SECCOUNT1  0x08
#define ATA_REG_LBA3       0x09
#define ATA_REG_LBA4       0x0A
#define ATA_REG_LBA5       0x0B
#define ATA_REG_CONTROL    0x0C
#define ATA_REG_ALTSTATUS  0x0C
#define ATA_REG_DEVADDRESS 0x0D


// Channels:
#define      ATA_PRIMARY      0x00
#define      ATA_SECONDARY    0x01
 
// Directions:
#define      ATA_READ      0x00
#define      ATA_WRITE     0x01


// 2 channels primary and secondary
struct IDEChannelRegisters 
{
	u16_t base;  	// I/O Base.
	u16_t ctrl;  	// Control Base
	u16_t bmide; 	// Bus Master IDE
	u8_t  nIEN;  	// nIEN (No Interrupt);
} channels[2];

// the IDE can contain up to 4 drives: 
struct ide_device 
{
   	u8_t  reserved;    // 0 (Empty) or 1 (This Drive really exists).
   	u8_t  channel;     // 0 (Primary Channel) or 1 (Secondary Channel).
   	u8_t  drive;       // 0 (Master Drive) or 1 (Slave Drive).
   	u16_t type;        // 0: ATA, 1:ATAPI.
   	u16_t signature;   // Drive Signature
   	u16_t capabilities;// Features.
   	u32_t commandSets; // Command Sets Supported.
   	u32_t size;        // Size in Sectors.
   	u8_t  model[41];   // Model in string.
} ide_devices[4];

void ata_install(PCI_Device* ide_dev);

void ide_initialize(u32_t BAR0, u32_t BAR1, u32_t BAR2, u32_t BAR3, u32_t BAR4);
void ide_initialise();


u8_t ide_read(u8_t channel, u8_t reg);

void ide_write(u8_t channel, u8_t reg, u8_t data);

void ide_read_buffer(u8_t channel, u8_t reg, u32_t buffer, u32_t quads);

u8_t ide_polling(u8_t channel, u32_t advanced_check);

u8_t ide_print_error(u32_t drive, u8_t err);

u8_t ide_ata_access(u8_t direction, u8_t drive, u32_t lba, u8_t numsects, u32_t edi);

void ata_install();
void ide_wait_irq();
u32_t ide_handler(u32_t context);

u8_t ide_atapi_read(u8_t drive, u32_t lba, u8_t numsects, u32_t edi);

int ide_read_sectors(u8_t drive, u8_t numsects, u32_t lba,u32_t edi);
int ide_write_sectors(u8_t drive, u8_t numsects, u32_t lba, u32_t edi);

int ide_atapi_eject(u8_t drive);
#endif
