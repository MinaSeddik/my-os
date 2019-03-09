#ifndef _SYSTEM_H
#define _SYSTEM_H

#include <types.h>
#include <thread.h>
#include <mutex.h>

#define GDT_ENTERIES_COUNT	3
#define IDT_ENTERIES_COUNT	256

#define GDT_KERNEL_CODE_SEL	1
#define GDT_KERNEL_DATA_SEL	2
#define GDT_KERNEL_STAK_SEL	2

//--------------------------------------------------------
#define HZ			1000		// to fire 1000 tick per second	
#define MAX_TICK_PREIOD		100		// 100 ticks
//--------------------------------------------------------

#define MAX_PROCESS_NAME	100

#define PROC_STACK_SIZE		0X4000		// 16 KB

#define MAX_PROCESS		333		// MAX number of process to run


#define EFLAGS_IF 		0x200		// enble interupt
#define EFLAGS_RESERVED 	0x2		// Reserved


#define PIC_MSTR_CTRL           0x20
#define PIC_MSTR_MASK           0x21

#define PIC_SLV_CTRL            0xA0
#define PIC_SLV_MASK            0xA1

struct gdt_entry
{
    u16_t limit_low;
    u16_t base_low;
    u8_t  base_middle;
    u8_t access;
    u8_t granularity;		// limit and flags ( bytes or pages )
    u8_t base_high;
} __attribute__((packed));


struct gdt_ptr
{
    u16_t limit;
    u32_t base;
} __attribute__((packed));


struct gdt_entry gdt[GDT_ENTERIES_COUNT];
struct gdt_ptr gdtp;


struct idt_entry
{
    u16_t base_low;
    u16_t sel;        		/* Our kernel segment goes here! */
    u8_t zero;     		/* This will ALWAYS be set to 0! */
    u8_t flags;       		/* Set using the above table! */
    u16_t base_high;
} __attribute__((packed));

struct idt_ptr
{
    u16_t limit;
    u32_t base;
} __attribute__((packed));

struct idt_entry idt[IDT_ENTERIES_COUNT];
struct idt_ptr idtp;


struct regs
{
    u32_t gs, fs, es, ds;      				/* pushed the segs last */
    u32_t edi, esi, ebp, esp, ebx, edx, ecx, eax;  	/* pushed by 'pusha' */
    u32_t int_no, err_code;    				/* our 'push byte #' and ecodes do this */
    u32_t eip, cs, eflags, useresp, ss;   		/* pushed by the processor automatically */ 
};


typedef struct time
{
	u8_t second;
	u8_t minute;
	u8_t hour;
	u8_t week_day;
	u8_t day;
	u8_t month;
	u8_t year;
	u8_t century;
}time_t;

void kprint(char* str);

u8_t inportb (u16_t _port);
void outportb (u16_t _port, u8_t _data);


void gdt_install();
void gdt_set_gate(int num, u32_t base, u32_t limit, u8_t access, u8_t gran);
extern void gdt_flush();		// assembly procedure


void idt_install();
void idt_set_gate(int num, u32_t base, u16_t sel, u8_t flags);
extern void idt_load();			// assembly procedure

void irq_remap();

// assembly procedures for exceptions handling
extern void isr0();
extern void isr1();
extern void isr2();
extern void isr3();
extern void isr4();
extern void isr5();
extern void isr6();
extern void isr7();
extern void isr8();
extern void isr9();
extern void isr10();
extern void isr11();
extern void isr12();
extern void isr13();
extern void isr14();
extern void isr15();
extern void isr16();
extern void isr17();
extern void isr18();
extern void isr19();
extern void isr20();
extern void isr21();
extern void isr22();
extern void isr23();
extern void isr24();
extern void isr25();
extern void isr26();
extern void isr27();
extern void isr28();
extern void isr29();
extern void isr30();
extern void isr31();


// assembly procedures for IRQ handling
extern void irq0();
extern void irq1();
extern void irq2();
extern void irq3();
extern void irq4();
extern void irq5();
extern void irq6();
extern void irq7();
extern void irq8();
extern void irq9();
extern void irq10();
extern void irq11();
extern void irq12();
extern void irq13();
extern void irq14();
extern void irq15();


void fault_handler(struct regs *r);
u32_t irq_handler(struct regs *r);


struct isr_handler
{
	char isr_name[100];
	u32_t (*handler)(u32_t context);
};

void timer_install();
void timer_init();
u32_t timer_handler(u32_t context);
void ksleep(u32_t milli);
void sleep(u32_t milli);
void irq_install_handler(int irq, u32_t (*handler)(u32_t context) );
u32_t gettickcount();



typedef struct _proc
{
	u32_t 	proc_id;
	u32_t	total_ticks_count;

	u32_t	ticks_assgined;
	
	u32_t	wait_ticks;

	u32_t*	base_stack;
	u32_t*	stack_pointer;
	u32_t	stack_size;

	u32_t	status;
	u32_t 	bsy_slot;

	u32_t 	wait_proc_id;

	mutex_t* locked_mutex;
	char 	name[MAX_PROCESS_NAME];

	THandler thread_handler;
  	void   (*threadfunc)(void*); 		/* 	Thread's main function */
  	void*  threadargs;           		/* 	Argument of the function */

} proc;


typedef proc thread;

proc process[MAX_PROCESS];

// ready process
proc* ready[MAX_PROCESS];

// blocked process
proc* blocked[MAX_PROCESS];

void  debug_register(struct regs *r);
void local_time(time_t* time);


void enable_irq(u32_t irq);
void disable_irq(u32_t irq);


#endif
