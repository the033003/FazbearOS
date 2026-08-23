#pragma once

#include <stdint.h>
#include <stddef.h>

void console_init(void);

void console_clear(void);

void console_putc(char character);
void console_write(const char* string);

void console_set_color(
    uint8_t foreground,
    uint8_t background
);

size_t console_width(void);
size_t console_height(void);

size_t console_column(void);
size_t console_row(void);

void console_cursor_left(void);
void console_cursor_right(void);

void console_backspace(void);

void console_newline(void);

void console_page_break(void);
