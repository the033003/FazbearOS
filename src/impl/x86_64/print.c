#include "print.h"
#include "console.h"

void print_clear(void)
{
    console_clear();
}

void print_char(char character)
{
    console_putc(character);
}

void print_str(const char* string)
{
    console_write(string);
}

void print_backspace(void)
{
    console_backspace();
}

void print_set_color(
    uint8_t foreground,
    uint8_t background
)
{
    console_set_color(
        foreground,
        background
    );
}
