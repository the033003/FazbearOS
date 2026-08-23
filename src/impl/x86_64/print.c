#include "print.h"

static const size_t NUM_COLS = 80;
static const size_t NUM_ROWS = 25;

struct Char {
    uint8_t character;
    uint8_t color;
};

static volatile struct Char* const buffer =
    (volatile struct Char*)0xB8000;

static size_t column = 0;
static size_t row = 0;

static uint8_t color =
    PRINT_COLOR_WHITE | (PRINT_COLOR_BLACK << 4);

static void clear_row(size_t row_index)
{
    struct Char empty = {
        .character = ' ',
        .color = color
    };

    for (size_t current_column = 0;
         current_column < NUM_COLS;
         current_column++) {

        buffer[
            current_column +
            NUM_COLS * row_index
        ] = empty;
    }
}

void print_clear(void)
{
    for (size_t current_row = 0;
         current_row < NUM_ROWS;
         current_row++) {

        clear_row(current_row);
    }

    column = 0;
    row = 0;
}

static void print_newline(void)
{
    column = 0;

    if (row < NUM_ROWS - 1) {
        row++;
        return;
    }

    for (size_t current_row = 1;
         current_row < NUM_ROWS;
         current_row++) {

        for (size_t current_column = 0;
             current_column < NUM_COLS;
             current_column++) {

            struct Char character =
                buffer[
                    current_column +
                    NUM_COLS * current_row
                ];

            buffer[
                current_column +
                NUM_COLS * (current_row - 1)
            ] = character;
        }
    }

    clear_row(NUM_ROWS - 1);
}

void print_char(char character)
{
    if (character == '\n') {
        print_newline();
        return;
    }

    if (character == '\r') {
        column = 0;
        return;
    }

    if (character == '\t') {
        print_char(' ');
        print_char(' ');
        print_char(' ');
        print_char(' ');
        return;
    }

    if (column >= NUM_COLS) {
        print_newline();
    }

    buffer[
        column +
        NUM_COLS * row
    ] = (struct Char) {
        .character = (uint8_t)character,
        .color = color
    };

    column++;
}

void print_str(const char* string)
{
    if (string == 0) {
        return;
    }

    for (size_t i = 0;
         string[i] != '\0';
         i++) {

        print_char(string[i]);
    }
}

void print_set_color(
    uint8_t foreground,
    uint8_t background
)
{
    color =
        foreground |
        (background << 4);
}
