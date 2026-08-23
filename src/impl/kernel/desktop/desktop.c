#include "desktop/desktop.h"

#include "graphics.h"
#include "mouse.h"

#define COLOR_DESKTOP      0x17172A
#define COLOR_TASKBAR      0x24243A
#define COLOR_WINDOW       0xC8C8D0
#define COLOR_BORDER       0xFFFFFF
#define COLOR_TITLEBAR     0x1769AA
#define COLOR_TITLEBAR_DARK 0x0E416B
#define COLOR_TEXT         0xFFFFFF
#define COLOR_CURSOR       0xFFFFFF
#define COLOR_CURSOR_EDGE  0x000000
#define COLOR_TASK_TEXT    0xFFFFFF

static window_t test_window;

static void draw_cursor(
    int x,
    int y
)
{
    /*
     * Classic arrow cursor.
     *
     * Draw the black outline first, then
     * the white interior.
     */

    static const char cursor[16][12] = {
        "X           ",
        "XX          ",
        "X.X         ",
        "X..X        ",
        "X...X       ",
        "X....X      ",
        "X.....X     ",
        "X......X    ",
        "X.......X   ",
        "X........X  ",
        "X.........X ",
        "X......XXXX ",
        "X...X       ",
        "X..XX       ",
        "X.X         ",
        "XX          "
    };

    for (int row = 0;
         row < 16;
         row++) {

        for (int col = 0;
             col < 12;
             col++) {

            char pixel =
                cursor[row][col];

            if (pixel == '\0' ||
                pixel == ' ') {
                continue;
            }

            graphics_put_pixel(
                x + col,
                y + row,
                COLOR_CURSOR_EDGE
            );
        }
    }

    static const char interior[16][12] = {
        "            ",
        "X           ",
        "X           ",
        "X.X         ",
        "X..X        ",
        "X...X       ",
        "X....X      ",
        "X.....X     ",
        "X......X    ",
        "X.......X   ",
        "X........X  ",
        "X......XX   ",
        "X...X       ",
        "X..X        ",
        "X.X         ",
        "            "
    };

    for (int row = 0;
         row < 16;
         row++) {

        for (int col = 0;
             col < 12;
             col++) {

            if (interior[row][col] != 'X') {
                continue;
            }

            graphics_put_pixel(
                x + col,
                y + row,
                COLOR_CURSOR
            );
        }
    }
}

void desktop_init(
    desktop_t *desktop,
    int width,
    int height
)
{
    if (desktop == 0) {
        return;
    }

    desktop->width =
        width;

    desktop->height =
        height;

    const struct mouse_state*
        mouse =
        mouse_get_state();

    if (mouse != 0) {

        desktop->mouse_x =
            mouse->x;

        desktop->mouse_y =
            mouse->y;

    } else {

        desktop->mouse_x =
            width / 2;

        desktop->mouse_y =
            height / 2;
    }

    if (desktop->mouse_x < 0) {
        desktop->mouse_x = 0;
    }

    if (desktop->mouse_y < 0) {
        desktop->mouse_y = 0;
    }

    if (desktop->mouse_x >= width) {
        desktop->mouse_x =
            width - 1;
    }

    if (desktop->mouse_y >= height) {
        desktop->mouse_y =
            height - 1;
    }

    desktop->mouse_left =
        false;

    desktop->previous_mouse_left =
        false;

    desktop->window_count =
        0;

    desktop->focused =
        0;

    desktop->dragging =
        0;

    desktop->terminal_open =
        false;

    for (int i = 0;
         i < DESKTOP_MAX_WINDOWS;
         i++) {

        desktop->windows[i] =
            0;
    }

    window_init(
        &test_window,
        "FazbearOS",
        width / 2 - 250,
        height / 2 - 150,
        500,
        300
    );

    desktop->windows[0] =
        &test_window;

    desktop->window_count =
        1;

    desktop->focused =
        &test_window;
}

void desktop_update(
    desktop_t *desktop
)
{
    if (desktop == 0) {
        return;
    }

    const struct mouse_state*
        mouse =
        mouse_get_state();

    if (mouse == 0) {
        return;
    }

    if (!mouse_event_available()) {
        return;
    }

    desktop_mouse_move(
        desktop,
        mouse->delta_x,
        mouse->delta_y
    );

    bool left =
        (mouse->buttons &
         MOUSE_BUTTON_LEFT) != 0;

    desktop_mouse_button(
        desktop,
        left
    );

    mouse_clear_event();
}

static void draw_window(
    window_t* window
)
{
    if (window == 0 ||
        !window->visible) {
        return;
    }

    graphics_fill_rect(
        window->x + 4,
        window->y + 4,
        window->width,
        window->height,
        0x080810
    );

    graphics_fill_rect(
        window->x,
        window->y,
        window->width,
        window->height,
        window->background
    );

    graphics_rect(
        window->x,
        window->y,
        window->width,
        window->height,
        window->border
    );

    graphics_fill_rect(
        window->x,
        window->y,
        window->width,
        28,
        window->titlebar
    );

    graphics_fill_rect(
        window->x,
        window->y + 27,
        window->width,
        1,
        COLOR_TITLEBAR_DARK
    );

    graphics_draw_text(
        window->x + 10,
        window->y + 8,
        window->title,
        COLOR_TEXT,
        window->titlebar,
        2
    );

    /*
     * Close button.
     */
    graphics_fill_rect(
        window->x +
            window->width - 24,
        window->y + 6,
        16,
        16,
        0xD83A56
    );

    graphics_draw_text(
        window->x +
            window->width - 21,
        window->y + 9,
        "X",
        COLOR_TEXT,
        0xD83A56,
        1
    );

    /*
     * Window contents.
     */
    graphics_fill_rect(
        window->x + 20,
        window->y + 55,
        window->width - 40,
        window->height - 75,
        0xE2E2EA
    );

    graphics_rect(
        window->x + 20,
        window->y + 55,
        window->width - 40,
        window->height - 75,
        0x9A9AAA
    );

    graphics_draw_text(
        window->x + 38,
        window->y + 78,
        "Welcome to FazbearOS",
        0x101020,
        0xE2E2EA,
        2
    );

    graphics_draw_text(
        window->x + 38,
        window->y + 108,
        "Desktop is running.",
        0x101020,
        0xE2E2EA,
        1
    );

    graphics_draw_text(
        window->x + 38,
        window->y + 130,
        "Move the mouse.",
        0x101020,
        0xE2E2EA,
        1
    );

    graphics_draw_text(
        window->x + 38,
        window->y + 152,
        "Drag this title bar.",
        0x101020,
        0xE2E2EA,
        1
    );
}

void desktop_render(
    desktop_t *desktop
)
{
    if (desktop == 0) {
        return;
    }

    if (!graphics_available()) {
        return;
    }

    /*
     * Everything is drawn into the back
     * buffer. The real framebuffer is not
     * touched until graphics_present().
     */
    graphics_clear(
        COLOR_DESKTOP
    );

    /*
     * Top branding.
     */
    graphics_fill_rect(
        0,
        0,
        desktop->width,
        42,
        0x11111F
    );

    graphics_draw_text(
        18,
        12,
        "FAZBEAROS",
        0x00AAFF,
        0x11111F,
        2
    );

    /*
     * Taskbar.
     */
    graphics_fill_rect(
        0,
        desktop->height - 40,
        desktop->width,
        40,
        COLOR_TASKBAR
    );

    graphics_fill_rect(
        0,
        desktop->height - 40,
        desktop->width,
        2,
        0x00AAFF
    );

    graphics_fill_rect(
        12,
        desktop->height - 32,
        100,
        24,
        0x1769AA
    );

    graphics_draw_text(
        26,
        desktop->height - 26,
        "START",
        COLOR_TASK_TEXT,
        0x1769AA,
        1
    );

    graphics_draw_text(
        desktop->width - 95,
        desktop->height - 26,
        "FAZBEAROS",
        COLOR_TASK_TEXT,
        COLOR_TASKBAR,
        1
    );

    /*
     * Windows.
     */
    for (int i = 0;
         i < desktop->window_count;
         i++) {

        draw_window(
            desktop->windows[i]
        );
    }

    /*
     * Cursor is always last, so it appears
     * above every window.
     */
    draw_cursor(
        desktop->mouse_x,
        desktop->mouse_y
    );
}

void desktop_mouse_move(
    desktop_t *desktop,
    int dx,
    int dy
)
{
    if (desktop == 0) {
        return;
    }

    desktop->mouse_x +=
        dx;

    desktop->mouse_y +=
        dy;

    if (desktop->mouse_x < 0) {
        desktop->mouse_x =
            0;
    }

    if (desktop->mouse_y < 0) {
        desktop->mouse_y =
            0;
    }

    if (desktop->mouse_x >= desktop->width) {
        desktop->mouse_x =
            desktop->width - 1;
    }

    if (desktop->mouse_y >= desktop->height) {
        desktop->mouse_y =
            desktop->height - 1;
    }

    if (desktop->dragging != 0) {

        window_t *window =
            desktop->dragging;

        window_move(
            window,
            desktop->mouse_x -
                window->drag_offset_x,
            desktop->mouse_y -
                window->drag_offset_y
        );
    }
}

void desktop_mouse_button(
    desktop_t *desktop,
    bool left
)
{
    if (desktop == 0) {
        return;
    }

    if (left &&
        !desktop->previous_mouse_left) {

        for (int i =
             desktop->window_count - 1;
             i >= 0;
             i--) {

            window_t *window =
                desktop->windows[i];

            if (window == 0 ||
                !window->visible) {
                continue;
            }

            if (!window_contains(
                    window,
                    desktop->mouse_x,
                    desktop->mouse_y)) {
                continue;
            }

            desktop->focused =
                window;

            for (int j = 0;
                 j < desktop->window_count;
                 j++) {

                if (desktop->windows[j] != 0) {
                    desktop->windows[j]->focused =
                        desktop->windows[j] ==
                        window;
                }
            }

            if (window_titlebar_contains(
                    window,
                    desktop->mouse_x,
                    desktop->mouse_y)) {

                desktop->dragging =
                    window;

                window->dragging =
                    true;

                window->drag_offset_x =
                    desktop->mouse_x -
                    window->x;

                window->drag_offset_y =
                    desktop->mouse_y -
                    window->y;
            }

            break;
        }
    }

    if (!left &&
        desktop->previous_mouse_left) {

        if (desktop->dragging != 0) {

            desktop->dragging->dragging =
                false;

            desktop->dragging =
                0;
        }
    }

    desktop->previous_mouse_left =
        left;
}
