
#include <system.h>
#include <string.h>
#include <stdio.h>

int cnt = 0;

extern int trace_me;

// here all IRQs is disabled excepts cascade
//u16_t irq_mask = 0xFFFB; 


// here all IRQs is enables
u16_t irq_mask = 0; 


//
// Set interrupt mask
//

static void set_intr_mask(u16_t mask) 
{
	outportb(PIC_MSTR_MASK, (u8_t) mask);
  	outportb(PIC_SLV_MASK, (u8_t) (mask >> 8));
}


//
// Enable IRQ
//

void enable_irq(u32_t irq) 
{
	irq_mask &= ~(1 << irq);
  	if (irq >= 8) irq_mask &= ~(1 << 2);
  	set_intr_mask(irq_mask);
}

//
// Disable IRQ
//

void disable_irq(u32_t irq) 
{
	irq_mask |= (1 << irq);
  	if ((irq_mask & 0xFF00) == 0xFF00) irq_mask |= (1 << 2);
  	set_intr_mask(irq_mask);
}


struct isr_handler isr_handlers[] =
{
    { "Division By Zero Exception"		/* exception 0 */	,	NULL } ,
    { "Debug Interrupt"				/* exception 1 */	,	NULL } ,
    { "Non Maskable Interrupt"			/* exception 2 */	,	NULL } ,
    { "Breakpoint Interrupt"			/* exception 3 */	,	NULL } ,
    { "Overflow Exception"			/* exception 4 */	,	NULL } ,
    { "Out of Bounds Exception"			/* exception 5 */	,	NULL } ,
    { "Invalid Opcode Exception"		/* exception 6 */	,	NULL } ,
    { "Coprocessor Not Available Exception"	/* exception 7 */	,	NULL } ,
    { "Double Fault Exception"			/* exception 8 */	,	NULL } ,
    { "Coprocessor Segment Overrun Exception"	/* exception 9 */	,	NULL } ,
    { "Bad TSS Exception"			/* exception 10 */	,	NULL } ,
    { "Segment Not Present Exception"		/* exception 11 */	,	NULL } ,
    { "Stack Fault Exception"			/* exception 12 */	,	NULL } ,
    { "General Protection Fault Exception"	/* exception 13 */	,	NULL } ,
    { "Page Fault Exception"			/* exception 14 */	,	NULL } ,
    { "Reserved Exception"			/* exception 15 */	,	NULL } ,
    { "Floating Point Exception"		/* exception 16 */	,	NULL } ,
    { "Alignment Check Exception"		/* exception 17 */	,	NULL } ,
    { "Machine Check Exception"			/* exception 18 */	,	NULL } ,
    { "Reserved Exception"			/* exception 19 */	,	NULL } ,
    { "Reserved Exception"			/* exception 20 */	,	NULL } ,
    { "Reserved Exception"			/* exception 21 */	,	NULL } ,
    { "Reserved Exception"			/* exception 22 */	,	NULL } ,
    { "Reserved Exception"			/* exception 23 */	,	NULL } ,
    { "Reserved Exception"			/* exception 24 */	,	NULL } ,
    { "Reserved Exception"			/* exception 25 */	,	NULL } ,
    { "Reserved Exception"			/* exception 26 */	,	NULL } ,
    { "Reserved Exception"			/* exception 27 */	,	NULL } ,
    { "Reserved Exception"			/* exception 28 */	,	NULL } ,
    { "Reserved Exception"			/* exception 29 */	,	NULL } ,
    { "Reserved Exception"			/* exception 30 */	,	NULL } ,
    { "Reserved Exception"			/* exception 31 */	,	NULL } ,


    { "System timer Interrupt"							/* IRQ 0 */	,	NULL } ,
    { "Keyboard controller Interrupt"						/* IRQ 1 */	,	NULL } ,
    { "Cascaded signals Interrupt"						/* IRQ 2 */	,	NULL } ,
    { "Serial port controller for COM2 Interrupt"				/* IRQ 3 */	,	NULL } ,
    { "Serial port controller for COM1 Interrupt"				/* IRQ 4 */	,	NULL } ,
    { "LPT port 2  or  sound card Interrupt"					/* IRQ 5 */	,	NULL } ,
    { "Floppy disk controller Interrupt"					/* IRQ 6 */	,	NULL } ,
    { "LPT port 1 Interrupt"							/* IRQ 7 */	,	NULL } ,
    { "RTC Timer Interrupt"							/* IRQ 8 */	,	NULL } ,
    { "The Interrupt is left open for the use of peripherals"			/* IRQ 9 */	,	NULL } ,
    { "The Interrupt is left open for the use of peripherals"			/* IRQ 10 */	,	NULL } ,
    { "The Interrupt is left open for the use of peripherals"			/* IRQ 11 */	,	NULL } ,
    { "Mouse on PS/2 connector Interrupt"					/* IRQ 12 */	,	NULL } ,
    { "Math co-processor Interrupt"						/* IRQ 13 */	,	NULL } ,
    { "Primary ATA channel Interrupt"						/* IRQ 14 */	,	NULL } ,
    { "Secondary ATA channel Interrupt"						/* IRQ 15 */	,	NULL } 

};

void idt_install()
{
	/* Sets the special IDT pointer up, just like in 'gdt.c' */
    	idtp.limit = (sizeof (struct idt_entry) * IDT_ENTERIES_COUNT) - 1;
    	idtp.base = ( u32_t ) &idt;

    	/* Clear out the entires IDT, initializing it to zeros */
    	memset(&idt, 0, sizeof(struct idt_entry) * IDT_ENTERIES_COUNT);

    	/* install Exception Handlers */
    	idt_set_gate(0, (unsigned)isr0, 0x08, 0x8E);
    	idt_set_gate(1, (unsigned)isr1, 0x08, 0x8E);
    	idt_set_gate(2, (unsigned)isr2, 0x08, 0x8E);
    	idt_set_gate(3, (unsigned)isr3, 0x08, 0x8E);
    	idt_set_gate(4, (unsigned)isr4, 0x08, 0x8E);
    	idt_set_gate(5, (unsigned)isr5, 0x08, 0x8E);
    	idt_set_gate(6, (unsigned)isr6, 0x08, 0x8E);
    	idt_set_gate(7, (unsigned)isr7, 0x08, 0x8E);
    	idt_set_gate(8, (unsigned)isr8, 0x08, 0x8E);
    	idt_set_gate(9, (unsigned)isr9, 0x08, 0x8E);
    	idt_set_gate(10, (unsigned)isr10, 0x08, 0x8E);
    	idt_set_gate(11, (unsigned)isr11, 0x08, 0x8E);
    	idt_set_gate(12, (unsigned)isr12, 0x08, 0x8E);
    	idt_set_gate(13, (unsigned)isr13, 0x08, 0x8E);
    	idt_set_gate(14, (unsigned)isr14, 0x08, 0x8E);
    	idt_set_gate(15, (unsigned)isr15, 0x08, 0x8E);
    	idt_set_gate(16, (unsigned)isr16, 0x08, 0x8E);
    	idt_set_gate(17, (unsigned)isr17, 0x08, 0x8E);
    	idt_set_gate(18, (unsigned)isr18, 0x08, 0x8E);
    	idt_set_gate(19, (unsigned)isr19, 0x08, 0x8E);
  	idt_set_gate(20, (unsigned)isr20, 0x08, 0x8E);
    	idt_set_gate(21, (unsigned)isr21, 0x08, 0x8E);
    	idt_set_gate(22, (unsigned)isr22, 0x08, 0x8E);
    	idt_set_gate(23, (unsigned)isr23, 0x08, 0x8E);
    	idt_set_gate(24, (unsigned)isr24, 0x08, 0x8E);
    	idt_set_gate(25, (unsigned)isr25, 0x08, 0x8E);
    	idt_set_gate(26, (unsigned)isr26, 0x08, 0x8E);
    	idt_set_gate(27, (unsigned)isr27, 0x08, 0x8E);
    	idt_set_gate(28, (unsigned)isr28, 0x08, 0x8E);
    	idt_set_gate(29, (unsigned)isr29, 0x08, 0x8E);
    	idt_set_gate(30, (unsigned)isr30, 0x08, 0x8E);
    	idt_set_gate(31, (unsigned)isr31, 0x08, 0x8E);

	/* install IRQs Handlers */
	irq_remap();

	idt_set_gate(32, (unsigned)irq0, 0x08, 0x8E);	/* IRQ 0 */
	idt_set_gate(33, (unsigned)irq1, 0x08, 0x8E);	/* IRQ 1 */
	idt_set_gate(34, (unsigned)irq2, 0x08, 0x8E);	/* IRQ 2 */
	idt_set_gate(35, (unsigned)irq3, 0x08, 0x8E);	/* IRQ 3 */
	idt_set_gate(36, (unsigned)irq4, 0x08, 0x8E);	/* IRQ 4 */
	idt_set_gate(37, (unsigned)irq5, 0x08, 0x8E);	/* IRQ 5 */
	idt_set_gate(38, (unsigned)irq6, 0x08, 0x8E);	/* IRQ 6 */
	idt_set_gate(39, (unsigned)irq7, 0x08, 0x8E);	/* IRQ 7 */
	idt_set_gate(40, (unsigned)irq8, 0x08, 0x8E);	/* IRQ 8 */
	idt_set_gate(41, (unsigned)irq9, 0x08, 0x8E);	/* IRQ 9 */
	idt_set_gate(42, (unsigned)irq10, 0x08, 0x8E);	/* IRQ 10 */
	idt_set_gate(43, (unsigned)irq11, 0x08, 0x8E);	/* IRQ 11 */
	idt_set_gate(44, (unsigned)irq12, 0x08, 0x8E);	/* IRQ 12 */
	idt_set_gate(45, (unsigned)irq13, 0x08, 0x8E);	/* IRQ 13 */
	idt_set_gate(46, (unsigned)irq14, 0x08, 0x8E);	/* IRQ 14 */
	idt_set_gate(47, (unsigned)irq15, 0x08, 0x8E);	/* IRQ 15 */

    	/* Points the processor's internal register to the new IDT */
    	idt_load();

	// install timer
	timer_install();

	// keyboard install
	printf("Installing Keyboard driver ...\n");
	keyboard_install();

	// ata install
	//printf("Installing ATA driver and detecting attached storage ...\n");
//	ata_install();

}

//  Normally, IRQs 0 to 7 are mapped to entries 8 to 15. 
//  This is a problem in protected mode, because IDT entry 8 is a
//  Double Fault! Without remapping, every time IRQ0 fires,
//  you get a Double Fault Exception, which is NOT actually
//  what's happening. We send commands to the Programmable
//  Interrupt Controller (PICs - also called the 8259's) in
//  order to make IRQ0 to 15 be remapped to IDT entries 32 to 47 
void irq_remap(void)
{
	// Master : 	command port = 0x20		data port = 0X21
	// Slave  :	command port = 0xA0		data port = 0xA1
    	outportb(0x20, 0x11);
    	outportb(0xA0, 0x11);

    	outportb(0x21, 0x20);	// master start offset will be 0x20 (32) in the inetrrupt table
    	outportb(0xA1, 0x28);	// slave start offset will be 0x28 (40) in the inetrrupt table
    
	outportb(0x21, 0x04);	// tell Master PIC that there is a slave PIC at IRQ2 (0000 0100)
    	outportb(0xA1, 0x02);	// tell Slave PIC its cascade identity (0000 0010)


    	outportb(0x21, 0x01);	// setting mode : 8086/88 (MCS-80/85) mode
    	outportb(0xA1, 0x01);	// setting mode : 8086/88 (MCS-80/85) mode

    	outportb(0x21, 0x0);	// restore saved masks, NO saved masks
    	outportb(0xA1, 0x0);	// restore saved masks, NO saved masks

	
}



void idt_set_gate(int num, u32_t base, u16_t sel, u8_t flags)
{

	idt[num].base_low = (u16_t) base;
	idt[num].base_high = (u16_t) ( base >> 16 );


	idt[num].sel = sel;	// code selector
	idt[num].zero = 0;	// always zero

	idt[num].flags = (u16_t) flags;

}



void fault_handler(struct regs *r)
{

	kprint(" Exception received  ... \n");


    	/* Is this a fault whose number is from 0 to 31? */
    	if (r->int_no < 32)
    	{
        	/* Display the description for the Exception that occurred.
	        *  In this tutorial, we will simply halt the system using an
        	*  infinite loop */
	        kprint(isr_handlers[r->int_no].isr_name);
		//debug_register(r);
        	kprint("\n Exception. System Halted!\n");

	        for (;;);
    	}
}



u32_t irq_handler(struct regs *r)
{

	u32_t this_context = &r;
	u32_t context = this_context;



	
//printf("Interrupt %d raised   %d   %d\n",r->int_no, r, context);

    	/* Is this a fault whose number is from 0 to 31? */
    	if (r->int_no >= 32 && r->int_no <= 47 )
    	{
        	/* Display the description for the Exception that occurred.
	        *  In this tutorial, we will simply halt the system using an
        	*  infinite loop */
//		if ( r->int_no != 32 && r->int_no != 33)
//			printf("Interrupt %d raised, %s handler = 0x%x\n", r->int_no, isr_handlers[r->int_no].isr_name ,isr_handlers[r->int_no].handler);


		if ( isr_handlers[r->int_no].handler )
		{


//printf("\nint = %d, bef context before = 0x%x\n", r->int_no, context);

			context = isr_handlers[r->int_no].handler(context);
/*
if (r->int_no == 32)
{
	printf (" [%d] ---> CS:EIP  0x%x:%d = %x \n", r->int_no, r->cs, r->eip, r->eip);
	struct regs *test = (struct regs *)context;
	printf (" [%d] ***> CS:EIP  0x%x:%d = %x \n", r->int_no, test->cs, test->eip, test->eip);
}

if (r->int_no == 32 && trace_me)
{

	printf (" [%d] ---> CS:EIP  0x%x:%d = %x \n", r->int_no, r->cs, r->eip, r->eip);
	struct regs* test = (struct regs *) (context+4);
	printf (" [%d] ***> CS:EIP  0x%x:%d = %x \n", test->int_no, test->cs, test->eip, test->eip);
	while(1);
}
*/

//printf("int = %d, aft context before = 0x%x\n", r->int_no, context);
//while(1);
		}

	        /* If the IDT entry that was invoked was greater than 40
		*  (meaning IRQ8 - 15), then we need to send an EOI to
    		*  the slave controller */
    		if (r->int_no >= 40)
        		outportb(PIC_SLV_CTRL, 0x20);	// EOI : End Of Inerrupt for slave PIC

    		/* In either case, we need to send an EOI to the master
    		*  interrupt controller too */
    		outportb(PIC_MSTR_CTRL, 0x20);	// EOI : End Of Inerrupt for master PIC

    	}

	return context;
}

void irq_install_handler(int irq, u32_t (*handler)(u32_t context) )
{

//	printf("Installing interrupt for irq = %d handler = 0x%x\n", irq , handler);
	isr_handlers[irq].handler = handler;	
}

void  debug_register(struct regs *r)
{

  printf ("\n%x:%d    SS     = %x\n", &(r->ss), &(r->ss) ,r->ss);
  printf ("%x:%d    useresp    = %x\n", &(r->useresp), &(r->useresp) ,r->useresp);
  printf ("%x:%d    EFLAGS = %x\n", &(r->eflags), &(r->eflags) ,r->eflags);	
  printf ("%x:%d    CS     = %x\n", &(r->cs), &(r->cs) ,r->cs);
  printf ("%x:%d    EIP    = %x\n", &(r->eip), &(r->eip) ,r->eip);

  printf ("%x:%d    error    = %x\n", &(r->err_code), &(r->err_code) ,r->err_code);
  printf ("%x:%d    INT    = %x\n", &(r->int_no), &(r->int_no) ,r->int_no);

  printf ("%x:%d    EAX    = %x\n", &(r->eax), &(r->eax) ,r->eax);
  printf ("%x:%d    ECX    = %x\n", &(r->ecx), &(r->ecx) ,r->ecx);
  printf ("%x:%d    EDX    = %x\n", &(r->edx), &(r->edx) ,r->edx);
  printf ("%x:%d    EBX    = %x\n", &(r->ebx), &(r->ebx) ,r->ebx);
  printf ("%x:%d    ESP    = %x\n", &(r->esp), &(r->esp) ,r->esp);
  printf ("%x:%d    EBP    = %x\n", &(r->ebp), &(r->ebp) ,r->ebp);
  printf ("%x:%d    ESI    = %x\n", &(r->esi), &(r->esi) ,r->esi);
  printf ("%x:%d    EDI    = %x\n", &(r->edi), &(r->edi) ,r->edi);

  printf ("%x:%d    DS     = %x\n", &(r->ds), &(r->ds) ,r->ds);
  printf ("%x:%d    ES     = %x\n", &(r->es), &(r->es) ,r->es);
  printf ("%x:%d    FS     = %x\n", &(r->fs), &(r->fs) ,r->fs);
  printf ("%x:%d    GS     = %x\n", &(r->gs), &(r->gs) ,r->gs);

}

