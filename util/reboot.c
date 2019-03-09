

#include <types.h>

#include <thread.h>
#include <util.h>
#include <stdio.h>
#include <memory.h>
#include <system.h>


/* keyboard interface IO port: data and control
   READ:   status port
   WRITE:  control register 
*/
#define KBRD_INTRFC 		0x64
 
/* keyboard interface bits */
#define KBRD_BIT_KDATA 		0 /* keyboard data is in buffer (output buffer is empty) (bit 0) */
#define KBRD_BIT_UDATA 		1 /* user data is in buffer (command buffer is empty) (bit 1) */
 
#define KBRD_IO 		0x60 /* keyboard IO port */
#define KBRD_RESET 		0xFE /* reset CPU command */
 
#define bit(n) (1<<(n)) /* Set bit n to 1 */
 
/* Check if bit n in flags is set */
#define check_flag(flags, n) ((flags) & bit(n))



int reboot(Th_Param* param)
{
	u8_t temp;
 
	printf("System Reboot ... \n");
	sleep(3000);

    	__asm__ __volatile__ ("cli");
 
    	/* Clear all keyboard buffers (output and command buffers) */
    	do
    	{
        	temp = inb(KBRD_INTRFC); /* empty user data */
        	if (check_flag(temp, KBRD_BIT_KDATA) != 0)
            		inportb(KBRD_IO); /* empty keyboard data */
    	} while (check_flag(temp, KBRD_BIT_UDATA) != 0);
 
    	outportb(KBRD_INTRFC, KBRD_RESET); /* pulse CPU reset line */
    	
	__asm__ __volatile__ ("hlt"); /* if that didn't work, halt the CPU */

	return 0;
}
