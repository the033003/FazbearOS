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

#define NIBBLE_TEXT_LEFT        18
#define NIBBLE_TEXT_TOP         58
#define NIBBLE_TEXT_RIGHT       18
#define NIBBLE_TEXT_BOTTOM      18

#define NIBBLE_CHARACTER_WIDTH  6
#define NIBBLE_CHARACTER_HEIGHT 7

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

    if (nibble->length >= NIBBLE_BUFFER_SIZE - 1) {
        return;
    }

    nibble->text[nibble->length] = character;
    nibble->length++;

    nibble->text[nibble->length] = '\0';
    nibble->dirty = true;
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
    if (
        nibble == 0 ||
        !nibble->active
    ) {
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
    if (
        nibble == 0 ||
        !nibble->active
    ) {
        return;
    }

    /*
     * Backspace.
     */
    if (character == '\b') {
        nibble_backspace(nibble);
        return;
    }

    /*
     * Arrow-key escape values are currently generated
     * by the keyboard driver as 0x01 through 0x04.
     *
     * Nibble is an append-style scratchpad for now, so
     * don't insert those control characters into the text.
     */
    if (
        character == '\x01' ||
        character == '\x02' ||
        character == '\x03' ||
        character == '\x04'
    ) {
        return;
    }

    /*
     * Tab becomes four spaces. This makes pasted-looking
     * indentation much more useful while keeping the
     * renderer simple.
     */
    if (character == '\t') {
        nibble_append(nibble, ' ');
        nibble_append(nibble, ' ');
        nibble_append(nibble, ' ');
        nibble_append(nibble, ' ');
        return;
    }

    /*
     * Newline is a real newline.
     */
    if (character == '\n') {
        nibble_append(nibble, '\n');
        return;
    }

    /*
     * Printable ASCII.
     */
    if (
        character >= 32 &&
        character <= 126
    ) {
        nibble_append(
            nibble,
            character
        );
    }
}

static int nibble_count_visible_lines(
    const nibble_t *nibble,
    const window_t *window
)
{
    if (
        nibble == 0 ||
        window == 0
    ) {
        return 1;
    }

    int right =
        window->x +
        window->width -
        NIBBLE_TEXT_RIGHT;

    int available_width =
        right -
        (window->x + NIBBLE_TEXT_LEFT);

    if (available_width < NIBBLE_CHARACTER_WIDTH) {
        available_width = NIBBLE_CHARACTER_WIDTH;
    }

    int characters_per_line =
        available_width /
        NIBBLE_CHARACTER_WIDTH;

    if (characters_per_line < 1) {
        characters_per_line = 1;
    }

    int lines = 1;
    int column = 0;

    for (int i = 0; i < nibble->length; i++) {
        char character = nibble->text[i];

        if (character == '\n') {
            lines++;
            column = 0;
            continue;
        }

        if (column >= characters_per_line) {
            lines++;
            column = 0;
        }

        column++;
    }

    return lines;
}

static void nibble_find_start_index(
    const nibble_t *nibble,
    const window_t *window,
    int visible_lines,
    int *start_index
)
{
    if (
        nibble == 0 ||
        window == 0 ||
        start_index == 0
    ) {
        return;
    }

    int right =
        window->x +
        window->width -
        NIBBLE_TEXT_RIGHT;

    int available_width =
        right -
        (window->x + NIBBLE_TEXT_LEFT);

    if (available_width < NIBBLE_CHARACTER_WIDTH) {
        available_width = NIBBLE_CHARACTER_WIDTH;
    }

    int characters_per_line =
        available_width /
        NIBBLE_CHARACTER_WIDTH;

    if (characters_per_line < 1) {
        characters_per_line = 1;
    }

    /*
     * First determine how many visual lines the entire
     * document occupies.
     */
    int total_lines =
        nibble_count_visible_lines(
            nibble,
            window
        );

    if (total_lines <= visible_lines) {
        *start_index = 0;
        return;
    }

    /*
     * Walk backwards from the end until only the number
     * of lines that fit in the editor remain.
     *
     * This gives Nibble simple automatic scrolling without
     * adding another field to nibble_t.
     */
    int lines_to_skip =
        total_lines -
        visible_lines;

    int line_count = 0;
    int column = 0;

    for (int i = 0; i < nibble->length; i++) {
        char character =
            nibble->text[i];

        if (character == '\n') {
            line_count++;

            if (line_count >= lines_to_skip) {
                *start_index = i + 1;
                return;
            }

            column = 0;
            continue;
        }

        if (column >= characters_per_line) {
            line_count++;

            if (line_count >= lines_to_skip) {
                *start_index = i;
                return;
            }

            column = 0;
        }

        column++;
    }

    *start_index = 0;
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

    const int left =
        window->x +
        NIBBLE_TEXT_LEFT;

    const int top =
        window->y +
        NIBBLE_TEXT_TOP;

    const int right =
        window->x +
        window->width -
        NIBBLE_TEXT_RIGHT;

    const int bottom =
        window->y +
        window->height -
        NIBBLE_TEXT_BOTTOM;

    const int character_width =
        NIBBLE_CHARACTER_WIDTH;

    const int character_height =
        NIBBLE_CHARACTER_HEIGHT;

    int available_width =
        right - left;

    int available_height =
        bottom - top;

    if (available_width < character_width) {
        available_width = character_width;
    }

    if (available_height < character_height) {
        available_height = character_height;
    }

    int characters_per_line =
        available_width /
        character_width;

    int visible_lines =
        available_height /
        character_height;

    if (characters_per_line < 1) {
        characters_per_line = 1;
    }

    if (visible_lines < 1) {
        visible_lines = 1;
    }

    int start_index = 0;

    nibble_find_start_index(
        nibble,
        window,
        visible_lines,
        &start_index
    );

    int cursor_x = left;
    int cursor_y = top;

    /*
     * Render only the portion of the buffer that fits.
     */
    for (
        int i = start_index;
        i < nibble->length;
        i++
    ) {
        char character =
            nibble->text[i];

        if (character == '\n') {
            cursor_x = left;
            cursor_y += character_height;

            if (
                cursor_y + character_height >
                bottom
            ) {
                break;
            }

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

    /*
     * Recalculate the cursor location from the complete
     * buffer. The cursor is always at the end of the text.
     */
    cursor_x = left;
    cursor_y = top;

    for (
        int i = start_index;
        i < nibble->length;
        i++
    ) {
        char character =
            nibble->text[i];

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

        cursor_x += character_width;
    }

    /*
     * Keep the cursor inside the editor when the document
     * ends exactly at a line boundary.
     */
    if (cursor_x >= right) {
        cursor_x = left;
        cursor_y += character_height;
    }

    if (
        cursor_y + character_height <=
        bottom
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

    /*
     * Main editor surface.
     */
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

    /*
     * App heading.
     */
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
