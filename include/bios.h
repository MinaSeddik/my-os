#ifndef _BIOS_H
#define _BIOS_H

#include <types.h>

#define BIOS_DATA_AREA		0x400		// in real mode

#define BASE_IO_COM1		0x00
#define BASE_IO_COM2		0x02
#define BASE_IO_COM3		0x04
#define BASE_IO_COM4		0x06

#define BASE_IO_LPT1		0x08
#define BASE_IO_LPT2		0x0A
#define BASE_IO_LPT3		0x0C
#define BASE_IO_LPT4		0x0E

#define RAM_MEM_SIZE		0x13		// Memory size in kb

#define EBDA_BASE_ADDR		0x0E

#define DISPLAY_MODE		0x49
#define BASE_IO_VGA		0x63

#define HARDDISK_DRIVERS_COUNT	0x75


void read_bios_config();

#endif
