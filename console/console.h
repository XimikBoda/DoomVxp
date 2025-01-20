#pragma once

void console_init(int _scr_w, int _scr_h, unsigned short* _scr_buf);
void console_put_char(char c);
void console_put_str(const char* str);
void console_set_clear_width(int width);