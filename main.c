#include <system.h>
#include <vga.h>
#include <string.h>
#include <stdio.h>
#include <bios.h>
#include <types.h>
#include <memory.h>
#include <multiboot.h>
#include <ata.h>
#include <filesystem.h>
#include <pci.h>
#include <thread.h>
#include <shell.h>


int init_network();

u32_t memory_size;
int kernel_loaded = 0;

// the entry point of the kernal
void kernal_start (multiboot_info_t* mbi, unsigned int magic)
{

	char msg[100] = "In the Name of Jesus Christ. \nWelcome to Mina Operating System v1.0\n\n";

	clear_screen();
	kprint(msg);

  	/* Check the magic number */
  	if (magic != MULTIBOOT_BOOTLOADER_MAGIC)
    	{
      		kprint("Multiboot failure.");
      		for (;;);
    	}

	if (mbi->flags & 0x000000001 ) // test 0 bit
	{
		memory_size = mbi->mem_lower + mbi->mem_upper;
	}
	else
	{
      		kprint("CanNOT determine Memory Size.");
      		for (;;);		
	}

	//printf("kernal starts at = 0x%x \n", 0 );
	//printf("kernal ends at = 0x%x \n", &end );
	//printf("kernal start at = 0x%x \n", &start_at );

	
	kprint("Installing GDT Global Descriptor Table ... 	");	
	gdt_install();
	kprint("[ OK ] \n");

	kprint("Installing IDT Interrupt Descriptor Table ... 	\n");	
	idt_install();

	kprint("\nInstalling Memory Manager ... 			\n");
	printf("Total Memory Size = %d MB.\n", memory_size / 1024 );
	memory_install();

	kprint("\nScanning PCIs ... \n");
	scan_pci();

	kprint("\nDetecting Storage Devices ... \n");
	pci_detect_device(PCI_CLASS_MASS_STORAGE, PCI_SUBCLASS_MASS_STORAGE);

	kprint("\nDetecting Ethernet Network Interfaces ... \n");
	pci_detect_device(PCI_CLASS_NETWORKING, PCI_SUBCLASS_ETHERNET);

	__asm__ __volatile__ ("sti");

	kprint("\nIDE Stotage Devices ...			");
	PCI_Device* ide_dev = pci_find_device(PCI_CLASS_MASS_STORAGE, PCI_SUBCLASS_MASS_STORAGE);
	if ( ide_dev )
	{
		kprint("[ OK ] \n");
		ata_install(ide_dev);
	}
	else
	{
		kprint("[ NO ] \n");
	}

	kprint("\nInitialize FileSystems ... \n");
	init_fileSystems();

//	kprint("\nInitialize Networking ... \n");
//	netif_init();
	THandler handlern = create_thread(&init_network, NULL);


	// init shell
	THandler handler = create_thread(&shell, NULL);

	//kprint("Back to the kernel ... \n");

	// infinite loop
	for(;;);

}

int init_network()
{
	kprint("\nInitialize Networking ... \n");
	netif_init();
}


