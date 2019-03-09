
#include <system.h>
#include <keyboard.h>
#include <stdlib.h>
#include <vga.h>

static u8_t kb_buffer[KEYBOARD_BUF_SIZE];		// circular buffer
int read_ptr, write_ptr;

// monitor control keys
bool caps_lock_on = FALSE;
bool num_lock_on = FALSE;
bool shift_on = FALSE;
bool control_on = FALSE;
bool alt_on = FALSE;


void write_char_into_buff(u8_t ch);

mutex_t* kb_mutex;

int is_full = 0;

static const u8_t kb_map_lower[KEY_BOARD_COUNT] =
{
    	0, 27, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', '\b', '\t', 'q', 'w', 'e', 'r', 't',
  	'y', 'u', 'i', 'o', 'p', '[', ']', '\n', 0  /* CTRL */, 'a', 's', 'd', 'f', 'g', 'h',
	'j', 'k', 'l', ';', '\'', '`', 0 /* LEFT SHIFT*/ , '\\', 'z', 'x', 'c', 'v', 'b', 'n', 'm', ',', 
	'.', '/', 0 /* RIGHT SHIFT */ , '*' /*55*/, 0 /* ALT */, ' ', 0 /* CAPS LOCK */ , 0 /* F1 */,
 	0 /* F2 */, 0 /* F3 */, 0 /* F4 */, 0 /* F5 */, 0/* F6 */ , 0 /* F7 */ , 0 /* F8 */, 0 /* F9 */, 
	0 /* F10 */ ,0 /* NUMLOCK */ ,0 /* SCROLL LOCK */ ,0 /* HOME */ ,0 /* UP */ ,0  /* PGUP 73*/,
	'-', 0, 0, 0, '+', 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};

static const u8_t kb_map_upper[KEY_BOARD_COUNT] =
{
    	0, 27, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', '\b', '\t', 'Q', 'W', 'E', 'R', 'T',
  	'Y', 'U', 'I', 'O', 'P', '[', ']', '\n', 0  /* CTRL */, 'A', 'S', 'D', 'F', 'G', 'H',
	'J', 'K', 'L', ';', '\'', '`', 0 /* LEFT SHIFT*/ , '\\', 'Z', 'X', 'C', 'V', 'B', 'N', 'M', ',', 
	'.', '/', 0 /* RIGHT SHIFT */ , '*' /*55*/, 0 /* ALT */, ' ', 0 /* CAPS LOCK */ , 0 /* F1 */,
 	0 /* F2 */, 0 /* F3 */, 0 /* F4 */, 0 /* F5 */, 0/* F6 */ , 0 /* F7 */ , 0 /* F8 */, 0 /* F9 */, 
	0 /* F10 */ ,0 /* NUMLOCK */ ,0 /* SCROLL LOCK */ ,0 /* HOME */ ,0 /* UP */ ,0  /* PGUP 73*/,
	'-', 0, 0, 0, '+', 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};

static const u8_t kb_map_shift_not_caps[KEY_BOARD_COUNT] =
{
    	0, 27, '!', '@', '#', '$', '%', '^', '&', '*', '(', ')', '_', '+', '\b', '\t', 'Q', 'W', 'E', 'R', 'T',
  	'Y', 'U', 'I', 'O', 'P', '{', '}', '\n', 0  /* CTRL */, 'A', 'S', 'D', 'F', 'G', 'H',
	'J', 'K', 'L', ':', '"', '|', 0 /* LEFT SHIFT*/ , '\\', 'Z', 'X', 'C', 'V', 'B', 'N', 'M', '<', 
	'>', '?', 0 /* RIGHT SHIFT */ , '*' /*55*/, 0 /* ALT */, ' ', 0 /* CAPS LOCK */ , 0 /* F1 */,
 	0 /* F2 */, 0 /* F3 */, 0 /* F4 */, 0 /* F5 */, 0/* F6 */ , 0 /* F7 */ , 0 /* F8 */, 0 /* F9 */, 
	0 /* F10 */ ,0 /* NUMLOCK */ ,0 /* SCROLL LOCK */ ,0 /* HOME */ ,0 /* UP */ ,0  /* PGUP 73*/,
	'-', 0, 0, 0, '+', 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};

static const u8_t kb_map_shift_caps[KEY_BOARD_COUNT] =
{
    	0, 27, '!', '@', '#', '$', '%', '^', '&', '*', '(', ')', '_', '+', '\b', '\t', 'q', 'w', 'e', 'r', 't',
  	'y', 'u', 'i', 'o', 'p', '{', '}', '\n', 0  /* CTRL */, 'a', 's', 'd', 'f', 'g', 'h',
	'j', 'k', 'l', ';', '\'', '`', 0 /* LEFT SHIFT*/ , '\\', 'z', 'x', 'c', 'v', 'b', 'n', 'm', '<', 
	'>', '?', 0 /* RIGHT SHIFT */ , '*' /*55*/, 0 /* ALT */, ' ', 0 /* CAPS LOCK */ , 0 /* F1 */,
 	0 /* F2 */, 0 /* F3 */, 0 /* F4 */, 0 /* F5 */, 0/* F6 */ , 0 /* F7 */ , 0 /* F8 */, 0 /* F9 */, 
	0 /* F10 */ ,0 /* NUMLOCK */ ,0 /* SCROLL LOCK */ ,0 /* HOME */ ,0 /* UP */ ,0  /* PGUP 73*/,
	'-', 0, 0, 0, '+', 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};


u32_t keyboard_handler(u32_t context)
{

    	u8_t scancode;
	u8_t ch;

    	/* Read from the keyboard's data buffer */
    	scancode = inportb(0x60);

    	if (scancode & 0x80)		// key released
    	{
        	scancode&=0x7F;

		//printf("scanCode = %d Released\n", scancode );

		switch(scancode)
		{
			case CONTROL_CODE:	// cntl key left and right
				control_on = FALSE;
				break;
			case ALT_CODE:	// Alt key left and right
				alt_on = FALSE;
				break;
			case SHIFT_LEFT_CODE:	// left shift
			case SHIFT_RIGHT_CODE:	// right shift
				shift_on = FALSE;
				break;
		}

    	}
    	else				// key pressed
    	{

		//printf("scanCode = %d Pressed CODE = %d  0x%x\n", scancode, kb_map_lower[scancode], kb_map_lower[scancode]);

		switch(scancode)
		{
			case CONTROL_CODE:		// cntl key left and right
				control_on = TRUE;
				break;
			case ALT_CODE:			// Alt key left and right
				alt_on = TRUE;
				break;
			case SHIFT_LEFT_CODE:		// left shift
			case SHIFT_RIGHT_CODE:		// right shift
				shift_on = TRUE;
				break;
			case CAPS_LOCK_CODE:			// caps lock
				caps_lock_on = !caps_lock_on;
				break;
			case NUM_LOCK_CODE:	// num lock
				num_lock_on = !num_lock_on;
				break;
			default :
				if ( control_on )	// send signal
				{
				}
				else
				{
				
					//ch = ( ! ( caps_lock_on ^ shift_on ) ) ? kb_map_lower[scancode]:kb_map_upper[scancode];
					if ( !caps_lock_on && !shift_on )
						ch = kb_map_lower[scancode];
					else if ( !caps_lock_on && shift_on )
						ch = kb_map_shift_not_caps[scancode];
					else if ( caps_lock_on && !shift_on )
						ch = kb_map_upper[scancode];
					else if ( caps_lock_on && shift_on )
						ch = kb_map_shift_caps[scancode];


        				//putch(ch);
					if ( ch )
						write_char_into_buff(ch);
				}
				break;
		}
			
    	}


	return context;
				
}



void keyboard_install()
{
	read_ptr = write_ptr = 0;
	mutex_init(kb_mutex);


    	irq_install_handler(33, keyboard_handler);
}


void write_char_into_buff(u8_t ch)
{

	kb_buffer[write_ptr++] = ch;
	write_ptr = write_ptr % KEYBOARD_BUF_SIZE;

	is_full = ( read_ptr == write_ptr ) ? 1 : 0;	

	notify_waiting_processess_keyboard_input();
}


u8_t get_char()
{
	char ch;
	if ( !is_full && read_ptr == write_ptr )		// the buffer is empty
		block_current_process_keyboard_input();

	ch = kb_buffer[read_ptr++];
	read_ptr = read_ptr % KEYBOARD_BUF_SIZE;

	return ch;	
}



