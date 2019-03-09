#ifndef _KEYBOARD_H
#define _KEYBOARD_H

#include <types.h>



#define KEY_BOARD_COUNT		128

#define CONTROL_CODE		29
#define ALT_CODE		56
#define SHIFT_RIGHT_CODE	54
#define SHIFT_LEFT_CODE		42
#define CAPS_LOCK_CODE		58
#define NUM_LOCK_CODE		69

#define LOWER_MODE	0
#define UPPER_MODE	1

#define KEYBOARD_BUF_SIZE	1024	// 1 KB


void keyboard_install();

u8_t get_char();


#endif
