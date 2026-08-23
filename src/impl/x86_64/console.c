#include "console.h"

static const size_t WIDTH = 80;
static const size_t HEIGHT = 25;

struct console_char {
    uint8_t character;
    uint8_t color;
};

static volatile struct console_char* const video =
    (volatile struct console_char*)0xB8000;

static size_t column = 0;
static size_t row = 0;

static uint8_t current_color =
    0x07;

static void clear_row(size_t row_index)
{
    for (size_t column_index = 0;
         column_index < WIDTH;
         column_index++) {

        video[
            row_index * WIDTH +
            column_index
        ] = (struct console_char) {
            .character = ' ',
            .color = current_color
        };
    }
}

static void scroll(void)
{
    for (size_t current_row = 1;
         current_row < HEIGHT;
         current_row++) {

        for (size_t current_column = 0;
             current_column < WIDTH;
             current_column++) {

            video[
                (current_row - 1) * WIDTH +
                current_column
            ] =
                video[
                    current_row * WIDTH +
                    current_column
                ];
        }
    }

    clear_row(HEIGHT - 1);

    row = HEIGHT - 1;
    column = 0;
}

void console_init(void)
{
    console_clear();
}

void console_clear(void)
{
    for (size_t row_index = 0;
         row_index < HEIGHT;
         row_index++) {

        clear_row(row_index);
    }

    column = 0;
    row = 0;
}

void console_putc(char character)
{
    if (character == '\n') {
        console_newline();
        return;
    }

    if (character == '\r') {
        column = 0;
        return;
    }

    if (character == '\t') {
        size_t spaces =
            4 - (column % 4);

        for (size_t i = 0;
             i < spaces;
             i++) {

            console_putc(' ');
        }

        return;
    }

    video[
        row * WIDTH +
        column
    ] = (struct console_char) {
        .character = (uint8_t)character,
        .color = current_color
    };

    column++;

    if (column >= WIDTH) {
        console_newline();
    }
}

void console_write(const char* string)
{
    if (string == 0) {
        return;
    }

    for (size_t i = 0;
         string[i] != '\0';
         i++) {

        console_putc(string[i]);
    }
}

void console_set_color(
    uint8_t foreground,
    uint8_t background
)
{
    current_color =
        foreground |
        (background << 4);
}

size_t console_width(void)
{
    return WIDTH;
}

size_t console_height(void)
{
    return HEIGHT;
}

size_t console_column(void)
{
    return column;
}

size_t console_row(void)
{
    return row;
}

void console_cursor_left(void)
{
    if (column > 0) {
        column--;
        return;
    }

    if (row > 0) {
        row--;
        column = WIDTH - 1;
    }
}

void console_cursor_right(void)
{
    column++;

    if (column >= WIDTH) {
        console_newline();
    }
}

void console_backspace(void)
{
    console_cursor_left();

    video[
        row * WIDTH +
        column
    ] = (struct console_char) {
        .character = ' ',
        .color = current_color
    };
}

void console_newline(void)
{
    column = 0;

    if (row < HEIGHT - 1) {
        row++;
    } else {
        scroll();
    }
}

void console_page_break(void)
{
    console_write(
        "\n-- More -- "
        "SPACE: next page | ENTER: next line | Q: quit --"
    );
}
