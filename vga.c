
#include <vga.h>#include <string.h>
#include <system.h>

#define START_VGA_BUF		0xB8000	// in the text mode
#define DEFAULT_ATTR 		0x0F	// black background and white foreground
#define TEXT_MODE_MAX_ROW	25
#define TEXT_MODE_MAX_COL	80
#define TEXT_MODE_MAX_CHAR	TEXT_MODE_MAX_ROW * TEXT_MODE_MAX_COL

#define TAB_SPACE			0x08

#define NEW_LINE_CHAR		'\n'
#define CARG_RET_CHAR		'\r'
#define BACK_SPACE_CHAR		0x08
#define TAB_CHAR		'\t'
#define SPACE_CHAR		0x20

#define ARROW_LEFT		0x20
#define ARROW_RIGHT		0x20


u16_t attrib = DEFAULT_ATTR;
u16_t x_pos, y_pos;

void init_vga()
{
    clear_screen();
}

void clear_screen()
{
	u16_t blank;

	restoredefaultcolor();
    	blank = SPACE_CHAR | (attrib << 8);

	memsetw((void *)START_VGA_BUF, blank, TEXT_MODE_MAX_CHAR);

    	x_pos = 0;
    	y_pos = 0;
    	move_csr();

}

void move_csr()
{
	unsigned target = y_pos * TEXT_MODE_MAX_COL + x_pos;

	// (0x3D4) index register
	// selects which register [0-11h] to write in
	// here we select reg = 14 for write    
	outportb(0x3D4, 14); 

	// (0x3D5) data register
	// so we will write the data ( target >> 8 ) into the selected index register ( 14 )
	outportb(0x3D5, target >> 8);

	// here we select reg = 15 for write  
	outportb(0x3D4, 15);

	// so we will write the data ( target ) into the selected index register ( 15 )
	outportb(0x3D5, target);

}

void scroll_screen()
{

	u16_t blank;

	restoredefaultcolor();
    	
	blank = (u16_t) SPACE_CHAR | (attrib << 8);

	memmove( (void *) START_VGA_BUF, (void *) ( START_VGA_BUF + (TEXT_MODE_MAX_COL * sizeof(u16_t) ) ), TEXT_MODE_MAX_COL * sizeof(u16_t) * (TEXT_MODE_MAX_ROW - 1) );

	memsetw( (void *) (START_VGA_BUF + ( TEXT_MODE_MAX_COL *  (TEXT_MODE_MAX_ROW - 1)  * sizeof(u16_t)) ), blank, TEXT_MODE_MAX_COL );

	y_pos = TEXT_MODE_MAX_ROW - 1;
	
}

	
void settextcolor(u8_t forecolor, u8_t backcolor)
{
    attrib = (backcolor << 4) | (forecolor & 0x0F);
}

void restoredefaultcolor()
{
	attrib = DEFAULT_ATTR;
}


/* Puts char on the screen */
void putch(u8_t ch)
{

	if ( NEW_LINE_CHAR == ch )	// handle new line
	{
		++y_pos;
		x_pos = 0;
	}
	else if ( CARG_RET_CHAR == ch )	// handle Carriage Return
	{
		x_pos = 0;
	}
	else if ( BACK_SPACE_CHAR == ch )	// handle back space 
	{
		if ( x_pos != 0 )	--x_pos;
		
		ch = SPACE_CHAR;
		u16_t* tmp =  (u16_t *) ( (u16_t *) START_VGA_BUF + ( y_pos * TEXT_MODE_MAX_COL ) + x_pos );
		*tmp = 	(u16_t) ( (u16_t) ch | (u16_t) DEFAULT_ATTR << 8 );
	}
	else if ( TAB_CHAR == ch ) 	// handle tab but point that will make it divisible by 8
	{
		x_pos = (x_pos + TAB_SPACE) & ~( TAB_SPACE - 1 );
	}
	
	else if( SPACE_CHAR <= ch )
    	{

		u16_t* tmp =  (u16_t *) ( (u16_t *) START_VGA_BUF + ( y_pos * TEXT_MODE_MAX_COL ) + x_pos );
		*tmp = 	(u16_t) ( (u16_t) ch | (u16_t) DEFAULT_ATTR << 8 );
		x_pos++;
    	}
	
	if ( x_pos >= TEXT_MODE_MAX_COL )
	{
		x_pos = 0;
		y_pos++;
	}
	
	if ( y_pos >= TEXT_MODE_MAX_ROW )
		scroll_screen();

	move_csr();

	
}
