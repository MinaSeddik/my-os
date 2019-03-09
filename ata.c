#include <ata.h>
#include <stdio.h>
#include <system.h>

u8_t ide_buf[2048] = {0};
unsigned static volatile char ide_irq_invoked = 0;
unsigned static char atapi_packet[12] = {0xA8, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};


//#define insl(port, buffer, count) \
//	         asm volatile("cld; rep; insl" :: "D" (buffer), "d" (port), "c" (count))

insl(u16_t port , void * address , int count)
{
        asm volatile("cld; rep insl" :
                "=D" (address), "=c" (count) :
                "d" (port), "0" (address), "1" (count) :
                "memory", "cc");
}


//void ide_initialise()
void ide_initialize(u32_t BAR0, u32_t BAR1, u32_t BAR2, u32_t BAR3, u32_t BAR4)
{
	int type, i, j, k, err, count = 0;

	// 1- Define I/O Ports which interface IDE Controller
//	channels[ATA_PRIMARY].base = 0x1F0;
//	channels[ATA_PRIMARY].ctrl = 0x3F4;
//	channels[ATA_SECONDARY].base = 0x170;
//	channels[ATA_SECONDARY].ctrl = 0x374;


	channels[ATA_PRIMARY].base = BAR0;
	channels[ATA_PRIMARY].ctrl = BAR1;
	channels[ATA_SECONDARY].base = BAR2;
	channels[ATA_SECONDARY].ctrl = BAR3;

	channels[ATA_PRIMARY].bmide = 0; // Bus Master IDE
	channels[ATA_SECONDARY].bmide = 8; // Bus Master IDE

	// 2- Disable IRQs temporarily
	ide_write(ATA_PRIMARY, ATA_REG_CONTROL, 2);
	ide_write(ATA_SECONDARY, ATA_REG_CONTROL, 2);

	// scanning IDE channels
	for (i=0;i<2;++i)		// 2 channels ( primary , secondary )
	{
		//for (masterslave = 0xA0; masterslave <= 0xB0; masterslave += 0x10)
		for (j=0;j<2;++j)	// master and slave
		{

			err = 0;

         		ide_devices[count].reserved = 0; // Assuming that no drive here.
 
         		// (I) Select Drive:
         		ide_write(i, ATA_REG_HDDEVSEL, 0xA0 | (j << 4)); // Select Drive.
         		ksleep(1); // Wait 1ms for drive select to work.
 
         		// (II) Send ATA Identify Command:
         		ide_write(i, ATA_REG_COMMAND, ATA_CMD_IDENTIFY);
         		ksleep(1); 
 
         		// (III) Polling:
			// Test weather device exists or not.
         		if (ide_read(i, ATA_REG_STATUS) == 0) 
				continue; // If Status = 0, No Device.

			// Check status of IDENTIFY command first
			u8_t cl = ide_read(i, ATA_REG_LBA1);
            		u8_t ch = ide_read(i, ATA_REG_LBA2);
			type = IDE_ATA;
			if (cl == 0x14 && ch == 0xEB)
			{
				// ATA IDENTIFY command aborted, this is SATA or ATAPI
				ide_write(i, ATA_REG_HDDEVSEL, 0xA0 | (j << 4));
				ide_write(i, ATA_REG_COMMAND, ATA_CMD_IDENTIFY_PACKET);
				type = IDE_ATAPI;			
			}


			while(1) 
			{
            			u8_t status = ide_read(i, ATA_REG_STATUS);
            			if ((status & ATA_SR_ERR)) {err = 1; break;} // If Err, Device is not ATA.
            			if (!(status & ATA_SR_BSY) && (status & ATA_SR_DRQ)) break; // Everything is right.
         		}

			if (err == 0)
			{

				// We found a drive! Add it to our list of found devices.
				ide_read_buffer(i, ATA_REG_DATA, (u32_t)ide_buf, 128);

				ide_devices[count].reserved = 1;
				ide_devices[count].type = type;
				ide_devices[count].channel = i;
				ide_devices[count].drive = j;
				ide_devices[count].signature = ((u16_t*)(ide_buf + ATA_IDENT_DEVICETYPE))[0];
				ide_devices[count].capabilities = ((u16_t*)(ide_buf + ATA_IDENT_CAPABILITIES))[0];
				ide_devices[count].commandSets = ((u16_t*)(ide_buf + ATA_IDENT_COMMANDSETS))[0];

				if (ide_devices[count].commandSets & (1<<26))
					ide_devices[count].size	= ((u64_t*)(ide_buf + ATA_IDENT_MAX_LBA_EXT)) [0] & 0xFFFFFFFFFFFull;
				else
					ide_devices[count].size	= ((u64_t*)(ide_buf + ATA_IDENT_MAX_LBA))[0] & (u64_t)0xFFFFFFFFFFFFull; //48 bits 

				for(k = ATA_IDENT_MODEL; k < (ATA_IDENT_MODEL+40); k+=2)
				{
					ide_devices[count].model[k - ATA_IDENT_MODEL] = ide_buf[k + 1];
					ide_devices[count].model[(k + 1) - ATA_IDENT_MODEL] = ide_buf[k];
				}

				// Backpeddle over the name removing trailing spaces
				k = 39;
				while (ide_devices[count].model[k] == ' ')
					ide_devices[count].model[k--] = 0;

				if (type == IDE_ATAPI)
					printf("%s: ATAPI CDROM, Drive = %s\n", ide_devices[count].model, ((ide_devices[count].drive) ? "master": "slave" ));
				else
					printf("%s: %d MB IDE HARD DISK, Drive = %s\n", ide_devices[count].model, ide_devices[count].size / 1024 / 2 , ((ide_devices[count].drive) ? "master": "slave" ));
				count++;
			}

		}	// end inner for loop
	}	// end outer for loop


	//register_interrupt_handler(IRQ14, ide_irq);
	//register_interrupt_handler(IRQ15, ide_irq);

}
 
u8_t ide_read(u8_t channel, u8_t reg) 
{
	u8_t result;
   
	if (reg > 0x07 && reg < 0x0C)
      		ide_write(channel, ATA_REG_CONTROL, 0x80 | channels[channel].nIEN);
   	
	if (reg < 0x08)
      		result = inb(channels[channel].base + reg - 0x00);
   	else if (reg < 0x0C)
      		result = inb(channels[channel].base  + reg - 0x06);
   	else if (reg < 0x0E)
      		result = inb(channels[channel].ctrl  + reg - 0x0A);
   	else if (reg < 0x16)
      		result = inb(channels[channel].bmide + reg - 0x0E);
   
	if (reg > 0x07 && reg < 0x0C)
      		ide_write(channel, ATA_REG_CONTROL, channels[channel].nIEN);
   
	return result;

}



void ide_write(u8_t channel, u8_t reg, u8_t data) 
{
   
	if (reg > 0x07 && reg < 0x0C)
      		ide_write(channel, ATA_REG_CONTROL, 0x80 | channels[channel].nIEN);
   
	if (reg < 0x08)
      		outb(channels[channel].base  + reg - 0x00, data);
   	else if (reg < 0x0C)
      		outb(channels[channel].base  + reg - 0x06, data);
   	else if (reg < 0x0E)
      		outb(channels[channel].ctrl  + reg - 0x0A, data);
   	else if (reg < 0x16)
      		outb(channels[channel].bmide + reg - 0x0E, data);
   
	if (reg > 0x07 && reg < 0x0C)
      		ide_write(channel, ATA_REG_CONTROL, channels[channel].nIEN);

}


void ide_read_buffer(u8_t channel, u8_t reg, u32_t buffer, u32_t quads)
{
	if (reg > 0x07 && reg < 0x0C)
      		ide_write(channel, ATA_REG_CONTROL, 0x80 | channels[channel].nIEN);
   
	asm("pushw %es; movw %ds, %ax; movw %ax, %es");
   
	if (reg < 0x08)
      		insl(channels[channel].base  + reg - 0x00, buffer, quads);
   	else if (reg < 0x0C)
      		insl(channels[channel].base  + reg - 0x06, buffer, quads);
   	else if (reg < 0x0E)
      		insl(channels[channel].ctrl  + reg - 0x0A, buffer, quads);
   	else if (reg < 0x16)
      		insl(channels[channel].bmide + reg - 0x0E, buffer, quads);
   
	asm("popw %es;");

	if (reg > 0x07 && reg < 0x0C)
      		ide_write(channel, ATA_REG_CONTROL, channels[channel].nIEN);
}


u8_t ide_polling(u8_t channel, u32_t advanced_check) 
{
	int i;
 
//printf("ide_polling 1 \n");
	// (I) Delay 400 nanosecond for BSY to be set:
   	// -------------------------------------------------
   	for(i = 0; i < 4; i++)
      		ide_read(channel, ATA_REG_ALTSTATUS); // Reading the Alternate Status port wastes 100ns; loop four times.
// printf("ide_polling 2 \n");
   	// (II) Wait for BSY to be cleared:
   	// -------------------------------------------------
   	while (ide_read(channel, ATA_REG_STATUS) & ATA_SR_BSY); // Wait for BSY to be zero.
// printf("ide_polling 3 \n");
   	if (advanced_check) 
	{
//printf("ide_polling 4 \n");
		u8_t state = ide_read(channel, ATA_REG_STATUS); // Read Status Register.
// printf("ide_polling 5 \n");
      		// (III) Check For Errors:
      		// -------------------------------------------------
      		if (state & ATA_SR_ERR)
         		return 2; // Error.
// printf("ide_polling 6 \n");
      		// (IV) Check If Device fault:
      		// -------------------------------------------------
      		if (state & ATA_SR_DF)
         		return 1; // Device Fault.
// printf("ide_polling 7 state = %d ... %d\n", state, ATA_SR_DRQ);
      		// (V) Check DRQ:
      		// -------------------------------------------------
      		// BSY = 0; DF = 0; ERR = 0 so we should check for DRQ now.
      		if ((state & ATA_SR_DRQ) == 0)
         		return 3; // DRQ should be set
//printf("ide_polling 8 \n");
 
   	}
 
   	return 0; // No Error.
 
}


u8_t ide_print_error(u32_t drive, u8_t err) 
{

	if (err == 0)
      		return err;
 
   	printf("IDE:");
   	if (err == 1) 
	{
		printf("- Device Fault\n     "); 
		err = 19;
	}
   	else if (err == 2) 
	{
      		u8_t st = ide_read(ide_devices[drive].channel, ATA_REG_ERROR);
      		if (st & ATA_ER_AMNF)	{	printf("- No Address Mark Found\n");   	err = 7;	}
      		if (st & ATA_ER_TK0NF)	{	printf("- No Media or Media Error\n");  err = 3;	}
      		if (st & ATA_ER_ABRT)   {	printf("- Command Aborted\n");      	err = 20;	}
      		if (st & ATA_ER_MCR)   	{	printf("- No Media or Media Error\n");  err = 3;	}
      		if (st & ATA_ER_IDNF)   {	printf("- ID mark not Found\n");      	err = 21;	}
      		if (st & ATA_ER_MC)   	{	printf("- No Media or Media Error\n");  err = 3;	}
      		if (st & ATA_ER_UNC)   	{	printf("- Uncorrectable Data Error\n"); err = 22;	}
      		if (st & ATA_ER_BBK)   	{	printf("- Bad Sectors\n");       	err = 13;	}
   	} 
	else  if (err == 3)           	{	printf("- Reads Nothing\n"); 		err = 23;	}
     	else  if (err == 4)  		{	printf("- Write Protected\n"); 		err = 8;	}
   
	printf("- [%s %s] %s\n",
      	(const char *[]){"Primary", "Secondary"}[ide_devices[drive].channel], // Use the channel as an index into the array
      	(const char *[]){"Master", "Slave"}[ide_devices[drive].drive], // Same as above, using the drive
      	ide_devices[drive].model);
 
   	return err;
}



/*

There is something needed to be expressed here, I have told before that Task-File is like that:

    * Register 0: [Word] Data Register. (Read-Write).
    * Register 1: [Byte] Error Register. (Read).
    * Register 1: [Byte] Features Register. (Write).
    * Register 2: [Byte] SECCOUNT0 Register. (Read-Write).
    * Register 3: [Byte] LBA0 Register. (Read-Write).
    * Register 4: [Byte] LBA1 Register. (Read-Write).
    * Register 5: [Byte] LBA2 Register. (Read-Write).
    * Register 6: [Byte] HDDEVSEL Register. (Read-Write).
    * Register 7: [Byte] Command Register. (Write).
    * Register 7: [Byte] Status Register. (Read). 

So each register between 2 to 5 should be 8-bits long. Really each of them are 16-bits long.

    * Register 2: [Bits 0-7] SECCOUNT0, [Bits 8-15] SECOUNT1
    * Register 3: [Bits 0-7] LBA0, [Bits 8-15] LBA3
    * Register 4: [Bits 0-7] LBA1, [Bits 8-15] LBA4
    * Register 5: [Bits 0-7] LBA2, [Bits 8-15] LBA5 

The word [(SECCOUNT1 << 8) | SECCOUNT0] expresses the number of sectors which can be read when you access by LBA48. When you access in CHS or LBA28, SECCOUNT0 only expresses number of sectors.

    * LBA0 makes up bits 0 : 7 of the LBA address when you read in LBA28 or LBA48; it can also be the sector number of CHS.
    * LBA1 makes up bits 8 : 15 of the LBA address when you read in LBA28 or LBA48; it can also be the low byte of the cylinder number of CHS.
    * LBA2 makes up bits 16 : 23 of the LBA address when you read in LBA28 or LBA48; it can also be the high byte of the cylinder number of CHS.
    * LBA3 makes up bits 24 : 31 of the LBA48 address.
    * LBA4 makes up bits 32 : 39 of the LBA48 address.
    * LBA5 makes up bits 40 : 47 of LBA48 address. 

Notice that the LBA0, 1 and 2 registers are 24 bits long in total, which is not enough for LBA28; the higher 4-bits can be written to the lower 4-bits of the HDDEVSEL register.

Also notice that if bit 6 of this register is set, we are going to use LBA, if not, we are going to use CHS. There is a mode which is called extended CHS. 

*/



/*
This function reads/writes sectors from ATA-Drive. 

	* If direction is 0 we are reading, else we are writing.
    	* drive is the drive number which can be from 0 to 3.
    	* lba is the LBA address which allows us to access disks up to 2TB.
    	* numsects is the number of sectors to be read, it is a char, as reading more than 256 sector immediately may performance issues. If numsects is 0, the ATA controller will know that we want 256 sectors.
    	* selector is the segment selector to read from, or write to.
    	* edi is the offset in that segment. 
*/
u8_t ide_ata_access(u8_t direction, u8_t drive, u32_t lba, u8_t numsects, u32_t edi)
{
	u8_t lba_mode, dma, cmd;
	u8_t lba_io[6];
	u32_t channel	= ide_devices[drive].channel; // Read the Channel.
	u32_t slavebit = ide_devices[drive].drive; // Read the Drive [Master/Slave]
	u32_t bus = channels[channel].base; // The Bus Base, like [0x1F0] which is also data port.
	u32_t words = 256; // Approx. all ATA-Drives have a sector-size of 512-byte.
	u16_t cyl, i; 

	u8_t head, sect, err;

	// disable IRQs
	ide_write(channel, ATA_REG_CONTROL, channels[channel].nIEN = (ide_irq_invoked = 0x0) + 0x02);

	// (I) Select one from LBA28, LBA48 or CHS;
	if (lba >= 0x10000000)
	{
//printf("- here 1\n");
		// LBA48:
		lba_mode = 2;
		lba_io[0] = (lba & 0x000000FF)>>0;
		lba_io[1] = (lba & 0x0000FF00)>>8;
		lba_io[2] = (lba & 0x00FF0000)>>16;
		lba_io[3] = (lba & 0xFF000000)>>24;
		lba_io[4] = 0; // LBA28 is integer, so 32-bits are enough to access 2TB
		lba_io[5] = 0; // LBA28 is integer, so 32-bits are enough to access 2TB.
		head = 0; // Lower 4-bits of HDDEVSEL are not used here.
	} 
	else if (ide_devices[drive].capabilities & 0x200)
	{
//printf("- here 2\n");
		// LBA28:
		lba_mode = 1;
		lba_io[0] = (lba & 0x000000FF)>>0;
		lba_io[1] = (lba & 0x0000FF00)>>8;
		lba_io[2] = (lba & 0x00FF0000)>>16;
		lba_io[3] = 0; // These Registers are not used here.
		lba_io[4] = 0;
		lba_io[5] = 0;
		head = (lba & 0x0F000000)>>24;
	}
	else
	{
//printf("- here 3\n");
		// CHS:
		lba_mode = 0;
		sect = (lba % 63) + 1;
		cyl = (lba + 1	- sect)/(16*63);
		lba_io[0] = sect;
		lba_io[1] = (cyl>>0) & 0xFF;
		lba_io[2] = (cyl>>8) & 0xFF;
		lba_io[3] = 0;
		lba_io[4] = 0;
		lba_io[5] = 0;
		head = (lba + 1 - sect) % (16 * 63) / (63); // Head number is written to HDDEVSEL lower 4-bits.
	}

	// (II) See if Drive Supports DMA or not;
	dma = 0; // Supports or doesn't, we don't support !!!


	// (III) Wait if the drive is busy;
//	while (ide_read(channel, ATA_REG_STATUS) & ATA_SR_BSY && get_ticks() - timer_start < 100); // Wait if Busy.
	while (ide_read(channel, ATA_REG_STATUS) & ATA_SR_BSY)
		ksleep(1);


	// (IV) Select Drive from the controller;
	if (lba_mode == 0)
		ide_write(channel, ATA_REG_HDDEVSEL, 0xA0 | (slavebit<<4) | head);	// Select Drive CHS.
	else
		ide_write(channel, ATA_REG_HDDEVSEL, 0xE0 | (slavebit<<4) | head);	// Select Drive LBA.


	// (V) Write Parameters;
	if (lba_mode == 2)
	{
		ide_write(channel, ATA_REG_SECCOUNT1, 0);
		ide_write(channel, ATA_REG_LBA3, lba_io[3]);
		ide_write(channel, ATA_REG_LBA4, lba_io[4]);
		ide_write(channel, ATA_REG_LBA5, lba_io[5]);
	}
	ide_write(channel, ATA_REG_SECCOUNT0, numsects);
	ide_write(channel, ATA_REG_LBA0, lba_io[0]);
	ide_write(channel, ATA_REG_LBA1, lba_io[1]);
	ide_write(channel, ATA_REG_LBA2, lba_io[2]);


	if (lba_mode == 0 && dma == 0 && direction == 0)
		cmd = ATA_CMD_READ_PIO;
	if (lba_mode == 1 && dma == 0 && direction == 0)
		cmd = ATA_CMD_READ_PIO;	
	if (lba_mode == 2 && dma == 0 && direction == 0)
		cmd = ATA_CMD_READ_PIO_EXT;	
	if (lba_mode == 0 && dma == 1 && direction == 0)
		cmd = ATA_CMD_READ_DMA;
	if (lba_mode == 1 && dma == 1 && direction == 0)
		cmd = ATA_CMD_READ_DMA;
	if (lba_mode == 2 && dma == 1 && direction == 0)
		cmd = ATA_CMD_READ_DMA_EXT;
	if (lba_mode == 0 && dma == 0 && direction == 1)
		cmd = ATA_CMD_WRITE_PIO;
	if (lba_mode == 1 && dma == 0 && direction == 1)
		cmd = ATA_CMD_WRITE_PIO;
	if (lba_mode == 2 && dma == 0 && direction == 1)
		cmd = ATA_CMD_WRITE_PIO_EXT;
	if (lba_mode == 0 && dma == 1 && direction == 1)
		cmd = ATA_CMD_WRITE_DMA;
	if (lba_mode == 1 && dma == 1 && direction == 1)
		cmd = ATA_CMD_WRITE_DMA;
	if (lba_mode == 2 && dma == 1 && direction == 1)
		cmd = ATA_CMD_WRITE_DMA_EXT;
	ide_write(channel, ATA_REG_COMMAND, cmd);				// Send the Command.


	if (dma)
		if (direction == 0){}		// DMA Read.
		else{} 				// DMA Write.
	else if (direction == 0)				// PIO Read.
		for (i = 0; i < numsects; i++)
		{
			if ((err = ide_polling(channel, 1)))
				return err; // Polling, then set error and exit if there is.
//printf("- here 4\n");
			//asm("pushw %es");
			asm("rep insw"::"c"(words), "d"(bus), "D"(edi)); // Receive Data.
			//asm("popw %es");
			edi += (words*2);
		}
	else
	{							// PIO Write.
		for (i = 0; i < numsects; i++)
		{
			if ((err = ide_polling(channel, 0))) // Polling.
				return err;
			//asm("pushw %ds");
			asm("rep outsw"::"c"(words), "d"(bus), "S"(edi)); // Send Data
			//asm("popw %ds");
			edi += (words*2);
		}

		ide_write(channel, ATA_REG_COMMAND, (char []) {	ATA_CMD_CACHE_FLUSH, ATA_CMD_CACHE_FLUSH, ATA_CMD_CACHE_FLUSH_EXT }[lba_mode]);

		if ((err = ide_polling(channel, 0))); // Polling.
			//return err;
	}

	return 0;
}


void ata_install(PCI_Device* ide_dev)
{

	// The device doesn't use IRQs, check if this is an Parallel ID
	if ( ide_dev->deviceif == 0x8A || ide_dev->deviceif == 0x80 )
	{
		ide_initialize(0x1F0, 0x3F4, 0x170, 0x374, 0); 
		//ide_initialise();

		irq_install_handler(46, ide_handler);
		irq_install_handler(47, ide_handler);
	}
	else
	{
		ide_initialize(ide_dev->bar[0], ide_dev->bar[1], ide_dev->bar[2], ide_dev->bar[3], ide_dev->bar[4]); 
		//ide_initialise();

		irq_install_handler(ide_dev->irq + 32, ide_handler);	
	}


}

void ide_wait_irq()
{
	while (!ide_irq_invoked);
	ide_irq_invoked = 0;
}


u32_t ide_handler(u32_t context)
{
	ide_irq_invoked = 1;

	return context;
}


u8_t ide_atapi_read(u8_t drive, u32_t lba, u8_t numsects, u32_t edi)
{
	u32_t channel = ide_devices[drive].channel;
	u32_t slavebit = ide_devices[drive].drive;
	u32_t bus = channels[channel].base;
	u32_t words = 2048 / 2 ; // Sector Size in Words, Almost All ATAPI Drives has a sector size of 2048 bytes.
	u8_t	err; 
	int i;


	// Enable IRQs:
	ide_write(channel, ATA_REG_CONTROL, channels[channel].nIEN = ide_irq_invoked = 0x0);

	// (I): Setup SCSI Packet:	which is 6 words (12 bytes) long
	// ------------------------------------------------------------------
	atapi_packet[ 0] = ATAPI_CMD_READ;
	atapi_packet[ 1] = 0x0;
	atapi_packet[ 2] = (lba>>24) & 0xFF;
	atapi_packet[ 3] = (lba>>16) & 0xFF;
	atapi_packet[ 4] = (lba>> 8) & 0xFF;
	atapi_packet[ 5] = (lba>> 0) & 0xFF;
	atapi_packet[ 6] = 0x0;
	atapi_packet[ 7] = 0x0;
	atapi_packet[ 8] = 0x0;
	atapi_packet[ 9] = numsects;
	atapi_packet[10] = 0x0;
	atapi_packet[11] = 0x0;

//printf("ide_atapi_read 1\n");
	// (II): Select the Drive:
	// ------------------------------------------------------------------
	ide_write(channel, ATA_REG_HDDEVSEL, slavebit<<4);

	// (III): Delay 400 nanosecond for select to complete:
	// ------------------------------------------------------------------
	ide_read(channel, ATA_REG_ALTSTATUS); // Reading Alternate Status Port wastes 100ns.
	ide_read(channel, ATA_REG_ALTSTATUS); // Reading Alternate Status Port wastes 100ns.
	ide_read(channel, ATA_REG_ALTSTATUS); // Reading Alternate Status Port wastes 100ns.
	ide_read(channel, ATA_REG_ALTSTATUS); // Reading Alternate Status Port wastes 100ns.

	// (IV): Inform the Controller that we use PIO mode:
	// ------------------------------------------------------------------
	ide_write(channel, ATA_REG_FEATURES, 0);		 // PIO mode.

	// (V): Tell the Controller the size of buffer:
	// ------------------------------------------------------------------
	ide_write(channel, ATA_REG_LBA1, (words * 2) & 0xFF);	// Lower Byte of Sector Size.
	ide_write(channel, ATA_REG_LBA2, (words * 2)>>8);	// Upper Byte of Sector Size.

	// (VI): Send the Packet Command:
	// ------------------------------------------------------------------
	ide_write(channel, ATA_REG_COMMAND, ATA_CMD_PACKET);		// Send the Command.

	// (VII): Waiting for the driver to finish or invoke an error:
	// ------------------------------------------------------------------
	if ((err = ide_polling(channel, 1)))
		return err;		 // Polling and return if error.

	// (VIII): Sending the packet data:
	// ------------------------------------------------------------------
	asm("rep	outsw"::"c"(6), "d"(bus), "S"(atapi_packet));	// Send Packet Data

	// Here we cannot poll. We should wait for an IRQ, then read the sectors. These two operations should be repeated for each sector. 

//printf("ide_atapi_read 2\n");
	// (IX): Recieving Data:
	// ------------------------------------------------------------------
	for (i = 0; i < numsects; i++)
	{
//printf("ide_atapi_read 2 bef\n");
		ide_wait_irq();					// Wait for an IRQ.
//printf("ide_atapi_read 2 after\n");
		if ((err = ide_polling(channel, 1)))
			return err;		// Polling and return if error.
//		asm("pushw %es");
		asm("rep insw"::"c"(words), "d"(bus), "D"(edi));// Receive Data.
//		asm("popw %es");
		edi += (words*2);
	}

//printf("ide_atapi_read 3\n");

	// (X): Waiting for an IRQ:
	// ------------------------------------------------------------------
	ide_wait_irq();

//printf("ide_atapi_read 3\n");

	// (XI): Waiting for BSY & DRQ to clear:
	// ------------------------------------------------------------------
	while (ide_read(channel, ATA_REG_STATUS) & (ATA_SR_BSY | ATA_SR_DRQ))
		ksleep(1);

	return 0; 
}

int ide_read_sectors(u8_t drive, u8_t numsects, u32_t lba,u32_t edi)
{
	int i;

	// 1: Check if the drive presents:
	// ==================================
	if (drive > 3 || ide_devices[drive].reserved == 0)
	{
		printf("Drive %d not found\n", drive);		// Drive Not Found!
		return 0;
	}

	// 2: Check if inputs are valid:
	// ==================================
	else if (((lba + numsects) > ide_devices[drive].size) && (ide_devices[drive].type == IDE_ATA))
	{
		printf("Seek to invalid position LBA = 0x%x\n", lba+numsects);	// Seeking to invalid position.
		return 0;
	}

	// 3: Read in PIO Mode through Polling & IRQs:
	// ============================================
	else
	{
		u8_t err;
		if (ide_devices[drive].type == IDE_ATA)
		{
//printf("ide_read_sectors 1\n", drive);
			err = ide_ata_access(ATA_READ, drive, lba, numsects, edi);
		}
		else if (ide_devices[drive].type == IDE_ATAPI)
		{
			for (i = 0; i < numsects; i++)
			{
//printf("ide_read_sectors 2\n", drive);
				err = ide_atapi_read(drive, lba + i, 1, edi + (i*2048));
			}
		}

		if (err)	ide_print_error(drive, err);

		return !err;
	}

	return 0;
}

int ide_write_sectors(u8_t drive, u8_t numsects, u32_t lba, u32_t edi)
{

	// 1: Check if the drive presents:
	// ==================================
	if (drive > 3 || ide_devices[drive].reserved == 0)
	{
		printf("Drive %d not found\n", drive);		// Drive Not Found!
		return 0;
	}

	// 2: Check if inputs are valid:
	// ==================================
	else if (((lba + numsects) > ide_devices[drive].size) && (ide_devices[drive].type == IDE_ATA))
	{
		printf("Seek to invalid position %d\n", lba+numsects);		// Seeking to invalid position.
		return 0;
	}

	// 3: Read in PIO Mode through Polling & IRQs:
	// ============================================
	else
	{
		u8_t err;
		if (ide_devices[drive].type == IDE_ATA)
			err = ide_ata_access(ATA_WRITE, drive, lba, numsects, edi);
		else
			if (ide_devices[drive].type == IDE_ATAPI)
				err = 4; // Write-Protected.
		
		if (err)	ide_print_error(drive, err);

		return !err;
	}

	return 0;
}

int ide_atapi_eject(u8_t drive)
{
	u32_t channel = ide_devices[drive].channel;
	u32_t slavebit = ide_devices[drive].drive;
	u32_t bus = channels[channel].base;
	u8_t err = 0;
	ide_irq_invoked = 0;

	// 1: Check if the drive presents:
	// ==================================
	if (drive > 3 || ide_devices[drive].reserved == 0)
	{
		printf("Drive %d not found\n", drive);
		return 0;
	}

	// 2: Check if drive isn't ATAPI:
	// ==================================
	else if (ide_devices[drive].type == IDE_ATA)
	{
		printf("Command aborted on drive %d\n", drive);		// Command Aborted.
		return 0;
	}

	// 3: Eject ATAPI Driver:
	// ============================================
	else
	{
		// Enable IRQs:
		ide_write(channel, ATA_REG_CONTROL, channels[channel].nIEN = ide_irq_invoked = 0x0);

		// (I): Setup SCSI Packet:
		// ------------------------------------------------------------------
		atapi_packet[ 0] = ATAPI_CMD_EJECT;
		atapi_packet[ 1] = 0x00;
		atapi_packet[ 2] = 0x00;
		atapi_packet[ 3] = 0x00;
		atapi_packet[ 4] = 0x02;
		atapi_packet[ 5] = 0x00;
		atapi_packet[ 6] = 0x00;
		atapi_packet[ 7] = 0x00;
		atapi_packet[ 8] = 0x00;
		atapi_packet[ 9] = 0x00;
		atapi_packet[10] = 0x00;
		atapi_packet[11] = 0x00;

		// (II): Select the Drive:
		// ------------------------------------------------------------------
		ide_write(channel, ATA_REG_HDDEVSEL, slavebit<<4);

		// (III): Delay 400 nanosecond for select to complete:
		// ------------------------------------------------------------------
		ide_read(channel, ATA_REG_ALTSTATUS); // Reading Alternate Status Port wastes 100ns.
		ide_read(channel, ATA_REG_ALTSTATUS); // Reading Alternate Status Port wastes 100ns.
		ide_read(channel, ATA_REG_ALTSTATUS); // Reading Alternate Status Port wastes 100ns.
		ide_read(channel, ATA_REG_ALTSTATUS); // Reading Alternate Status Port wastes 100ns.

		// (IV): Send the Packet Command:
		// ------------------------------------------------------------------
		ide_write(channel, ATA_REG_COMMAND, ATA_CMD_PACKET);		// Send the Command.

		// (V): Waiting for the driver to finish or invoke an error:
		// ------------------------------------------------------------------
		if (!(err = ide_polling(channel, 1)))
		{
			asm("rep outsw"::"c"(6), "d"(bus), "S"(atapi_packet));// Send Packet Data
			ide_wait_irq();					// Wait for an IRQ.
			err = ide_polling(channel, 1);			// Polling and get error code.
			if (err == 3)	err = 0; // DRQ is not needed here.
		}

		if (err)	ide_print_error(drive, err);
		
		return !err;
	}
	
	return 0;
}

