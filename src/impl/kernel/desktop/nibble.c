#include "desktop/nibble.h"

#include "graphics.h"
#include "keyboard.h"

#define NIBBLE_COLOR_BACKGROUND 0x101522
#define NIBBLE_COLOR_PANEL      0x171E2E
#define NIBBLE_COLOR_BORDER     0x2D3A52
#define NIBBLE_COLOR_TEXT       0xE8EEF7
#define NIBBLE_COLOR_ACCENT     0x55D6BE
#define NIBBLE_COLOR_MUTED      0x8290A8
#define NIBBLE_COLOR_CURSOR     0x55D6BE

static void nibble_clear(
    nibble_t *nibble
)
{
    if (nibble == 0) {
        return;
    }

    for (int i = 0; i < NIBBLE_BUFFER_SIZE; i++) {
        nibble->text[i] = '\0';
    }

    nibble->length = 0;
    nibble->dirty = true;
}

static void nibble_append(
    nibble_t *nibble,
    char character
)
{
    if (nibble == 0) {
        return;
    }

    if (
        nibble->length <
        NIBBLE_BUFFER_SIZE - 1
    ) {
        nibble->text[nibble->length] = character;
        nibble->length++;
        nibble->text[nibble->length] = '\0';
        nibble->dirty = true;
    }
}

static void nibble_backspace(
    nibble_t *nibble
)
{
    if (nibble == 0) {
        return;
    }

    if (nibble->length <= 0) {
        return;
    }

    nibble->length--;

    nibble->text[nibble->length] = '\0';
    nibble->dirty = true;
}

void nibble_init(
    nibble_t *nibble
)
{
    if (nibble == 0) {
        return;
    }

    nibble_clear(nibble);

    nibble->active = true;
    nibble->dirty = true;
}

void nibble_update(
    nibble_t *nibble
)
{
    if (nibble == 0 || !nibble->active) {
        return;
    }

    while (keyboard_available()) {
        nibble_handle_key(
            nibble,
            keyboard_get_char()
        );
    }
}

void nibble_handle_key(
    nibble_t *nibble,
    char character
)
{
    if (nibble == 0 || !nibble->active) {
        return;
    }

    if (character == '\b') {
        nibble_backspace(nibble);
        return;
    }

    if (character == '\x01' ||
        character == '\x02' ||
        character == '\x03' ||
        character == '\x04') {
        return;
    }

    if (character == '\t') {
        nibble_append(nibble, ' ');
        nibble_append(nibble, ' ');
        return;
    }

    if (
        character == '\n' ||
        character == ' ' ||
        (
            character >= '!' &&
            character <= '~'
        )
    ) {
        nibble_append(nibble, character);
    }
}

static void draw_wrapped_text(
    const nibble_t *nibble,
    const window_t *window
)
{
    if (
        nibble == 0 ||
        window == 0
    ) {
        return;
    }

    const int left = window->x + 18;
    const int top = window->y + 58;

    const int right =
        window->x +
        window->width -
        18;

    const int bottom =
        window->y +
        window->height -
        18;

    const int character_width = 6;
    const int character_height = 7;

    int cursor_x = left;
    int cursor_y = top;

    for (int i = 0; i < nibble->length; i++) {
        char character = nibble->text[i];

        if (character == '\n') {
            cursor_x = left;
            cursor_y += character_height;
            continue;
        }

        if (
            cursor_x + character_width >
            right
        ) {
            cursor_x = left;
            cursor_y += character_height;
        }

        if (
            cursor_y + character_height >
            bottom
        ) {
            break;
        }

        graphics_draw_char(
            cursor_x,
            cursor_y,
            character,
            NIBBLE_COLOR_TEXT,
            NIBBLE_COLOR_BACKGROUND,
            1
        );

        cursor_x += character_width;
    }

    if (
        nibble->active &&
        cursor_y + character_height <= bottom
    ) {
        graphics_fill_rect(
            cursor_x,
            cursor_y,
            2,
            6,
            NIBBLE_COLOR_CURSOR
        );
    }
}

void nibble_render(
    const nibble_t *nibble,
    const window_t *window
)
{
    if (
        nibble == 0 ||
        window == 0 ||
        !window->visible ||
        window->minimized
    ) {
        return;
    }

    int content_width =
        window->width - 24;

    int content_height =
        window->height - 52;

    if (content_width < 20) {
        content_width = 20;
    }

    if (content_height < 20) {
        content_height = 20;
    }

    graphics_fill_rect(
        window->x + 10,
        window->y + 38,
        content_width,
        content_height,
        NIBBLE_COLOR_BACKGROUND
    );

    graphics_rect(
        window->x + 10,
        window->y + 38,
        content_width,
        content_height,
        NIBBLE_COLOR_BORDER
    );

    graphics_draw_text(
        window->x + 18,
        window->y + 43,
        "NIBBLE",
        NIBBLE_COLOR_ACCENT,
        NIBBLE_COLOR_BACKGROUND,
        1
    );

    graphics_draw_text(
        window->x + 65,
        window->y + 43,
        "tiny scratchpad",
        NIBBLE_COLOR_MUTED,
        NIBBLE_COLOR_BACKGROUND,
        1
    );

    draw_wrapped_text(
        nibble,
        window
    );
}
