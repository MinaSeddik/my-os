#include<bios.h>

#include <stdio.h>


u16_t base_io_com1;
u16_t base_io_com2;
u16_t base_io_com3;
u16_t base_io_com4;

u16_t base_io_lpt1;
u16_t base_io_lpt2;
u16_t base_io_lpt3;
u16_t base_io_lpt4;

u16_t memory_size;

u16_t EBDA_address;

u8_t display_mode;
u16_t base_io_vga;

u8_t harddisk_count;


void read_bios_config()
{
	
	base_io_com1 = * (u16_t*) (BIOS_DATA_AREA + BASE_IO_COM1);
	base_io_com2 = * (u16_t*) (BIOS_DATA_AREA + BASE_IO_COM2);
	base_io_com3 = * (u16_t*) (BIOS_DATA_AREA + BASE_IO_COM3);
	base_io_com4 = * (u16_t*) (BIOS_DATA_AREA + BASE_IO_COM4);

	base_io_lpt1 = * (u16_t*) (BIOS_DATA_AREA + BASE_IO_LPT1);
	base_io_lpt2 = * (u16_t*) (BIOS_DATA_AREA + BASE_IO_LPT2);
	base_io_lpt3 = * (u16_t*) (BIOS_DATA_AREA + BASE_IO_LPT3);
	base_io_lpt4 = * (u16_t*) (BIOS_DATA_AREA + BASE_IO_LPT4);

	EBDA_address = * (u16_t*) (BIOS_DATA_AREA + EBDA_BASE_ADDR);

	memory_size = * (u16_t*) (BIOS_DATA_AREA + RAM_MEM_SIZE);

	display_mode = * (u8_t*) (BIOS_DATA_AREA + DISPLAY_MODE);
	base_io_vga = * (u16_t*) (BIOS_DATA_AREA + BASE_IO_VGA);
	harddisk_count = * (u8_t*) (BIOS_DATA_AREA + HARDDISK_DRIVERS_COUNT);

/*
	printf("\nBase IO for COM1 = 0x%x", base_io_com1);
	printf("\nBase IO for COM2 = 0x%x", base_io_com2);
	printf("\nBase IO for COM3 = 0x%x", base_io_com3);
	printf("\nBase IO for COM4 = 0x%x", base_io_com4);
	
	printf("\nBase IO for LPT1 = 0x%x", base_io_lpt1);
	printf("\nBase IO for LPT2 = 0x%x", base_io_lpt2);
	printf("\nBase IO for LPT3 = 0x%x", base_io_lpt3);
	printf("\nBase IO for LPT4 = 0x%x", base_io_lpt4);

	printf("\nEBDA (Extended Bios Data Area) Address = 0x%x", EBDA_address);

	printf("\nLower Memory Size = %d Kilobytes", memory_size);
	printf("\nDisplay mode = %d", display_mode);
	printf("\nBase IO for VGA = 0x%x", base_io_vga);

	printf("\nDetected Hard disks = %d", harddisk_count);

	printf("\n\n");

*/
}
