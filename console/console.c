#include "console.h"
#include "Profont6x11.h"

#include <vmgraph.h>

const int char_width = 6;
const int char_height = 11;

static int scr_width = 0;
static int scr_clear_width = 0;
static int scr_height = 0;

static int c_w = 0;
static int c_x = 0;

static char need_scroll = 0;

static VMUINT16* scr_buf = 0;

int console_get_c_w() {
	return c_w;
}

void console_set_clear_width(int width) {
	scr_clear_width = width;
}

void console_init(int _scr_w, int _scr_h, unsigned short* _scr_buf) {
	scr_width = _scr_w;
	scr_clear_width = _scr_w;
	scr_height = _scr_h;
	scr_buf = _scr_buf;
	c_w = scr_height / char_width;
}

static void scroll_up() {
	if (scr_clear_width - char_height <= 0)
		return;
	for (int i = 0; i < scr_height; ++i) {
		VMUINT16* scr_line = scr_buf + scr_width * i;
		memmove(scr_line + char_height, scr_line, (scr_clear_width - char_height) * 2);
		memset(scr_line, 0, char_height * 2);
	}
}

static void draw_xy_char(int x, int y, char c) {
	const unsigned char* font_ch = ProFont6x11 + 5 + 12 * c + 1;
	const unsigned short textcolor = 0xFFFF, backcolor = 0;

	for (int i = 0; i < char_height; ++i) {
		for (int j = 0; j < char_width; ++j)
			scr_buf[x + (char_height - 1 - i) + (y + j) * scr_width] = ((((*font_ch) >> j) & 1) ? textcolor : backcolor);
		++font_ch;
	}
}

void console_put_char(char c) {
	if (need_scroll) {
		scroll_up();
		need_scroll = 0;
	}
	if (c_x == c_w || c == '\n') {
		need_scroll = 1;
		c_x = 0;
	}
	if (c == '\n') {
		if (scr_clear_width == scr_width)
			flush_layer();
		return;
	}
	draw_xy_char(0, c_x * char_width, c);
	++c_x;
}

void console_put_str(const char* str) {
	while (*str) {
		console_put_char(*str);
		++str;
	}
	if(scr_clear_width == scr_width)
		flush_layer();
}