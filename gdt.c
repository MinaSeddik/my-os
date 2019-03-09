
#include <system.h>

void gdt_install()
{
	/* Setup the GDT pointer and limit */
    	gdtp.limit = (sizeof(struct gdt_entry) * GDT_ENTERIES_COUNT) - 1;
    	gdtp.base = ( u32_t ) &gdt;

	// the first descriptor [0] should be 0
	gdt_set_gate(0, 0, 0, 0, 0);

	// the second descriptor [1] is for kernal code selector
	gdt_set_gate(GDT_KERNEL_CODE_SEL, 0, 0xFFFFFFFF, 0x9A, 0xCF);

	// the third descriptor [2] is for kernal data selector
	gdt_set_gate(GDT_KERNEL_DATA_SEL, 0, 0xFFFFFFFF, 0x92, 0xCF);


	// install kernal gdt
	gdt_flush();
	
}

void gdt_set_gate(int num, u32_t base, u32_t limit, u8_t access, u8_t gran)
{

	gdt[num].base_low = (u16_t) base;
	gdt[num].base_middle = (u8_t) ( (base >> 16 ) & 0xFF );
	gdt[num].base_high = (u8_t) ( (base >> 24) & 0xFF );	

	gdt[num].limit_low = (u16_t) limit;
	gdt[num].granularity = (u8_t) ( (limit >> 16) & 0x0F );

	gdt[num].granularity|= (gran & 0xF0);
    	gdt[num].access = access;
		
}
