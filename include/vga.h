#ifndef _VGA_H
#define _VGA_H

#include <types.h>

void init_vga();

void clear_screen();

void move_csr();

void scroll_screen();

void settextcolor(u8_t forecolor, u8_t backcolor);

void restoredefaultcolor();

void putch(u8_t ch);

#endif
