#include "desktop/desktop.h"
#include "desktop/window.h"

#include "graphics/framebuffer.h"

#define COLOR_DESKTOP  0x00101824
#define COLOR_TASKBAR  0x00151d28
#define COLOR_BORDER   0x003c5065
#define COLOR_TITLE    0x00212d3a
#define COLOR_TEXT     0x00d8e4ef
#define COLOR_CURSOR   0x00ffffff
#define COLOR_BLACK    0x00000000

static window_t terminal_window;

static void fill_rect(
    int x,
    int y,
    int width,
    int height,
    uint32_t color
) {
    int px;
    int py;

    for (py = y; py < y + height; ++py) {
        for (px = x; px < x + width; ++px) {
            framebuffer_putpixel(px, py, color);
        }
    }
}

static void draw_cursor(
    int x,
    int y
) {
    static const char cursor[] = {
        1, 1, 0, 0, 0,
        1, 1, 1, 0, 0,
        1, 1, 1, 1, 0,
        1, 1, 1, 1, 1,
        1, 1, 1, 1, 1,
        1, 1, 1, 0, 0,
        1, 1, 0, 0, 0,
        1, 0, 0, 0, 0
    };

    int row;
    int col;

    for (row = 0; row < 8; ++row) {
        for (col = 0; col < 5; ++col) {
            if (cursor[row * 5 + col]) {
                framebuffer_putpixel(
                    x + col,
                    y + row,
                    COLOR_CURSOR
                );
            }
        }
    }
}

static void render_window(
    const window_t *window
) {
    if (!window || !window->visible) {
        return;
    }

    fill_rect(
        window->x,
        window->y,
        window->width,
        window->height,
        window->border
    );

    fill_rect(
        window->x + 1,
        window->y + 1,
        window->width - 2,
        window->height - 2,
        window->background
    );

    fill_rect(
        window->x + 1,
        window->y + 1,
        window->width - 2,
        22,
        window->focused
            ? 0x002b3d50
            : window->titlebar
    );

    /*
     * Window controls.
     *
     * We intentionally keep these graphical-only for now.
     * Close/minimize behavior comes after the basic compositor.
     */
    fill_rect(
        window->x + window->width - 54,
        window->y + 7,
        8,
        8,
        0x00d0a050
    );

    fill_rect(
        window->x + window->width - 34,
        window->y + 7,
        8,
        8,
        0x00d05050
    );
}

void desktop_init(
    desktop_t *desktop,
    int width,
    int height
) {
    if (!desktop) {
        return;
    }

    desktop->width = width;
    desktop->height = height;

    desktop->mouse_x = width / 2;
    desktop->mouse_y = height / 2;

    desktop->mouse_left = false;
    desktop->previous_mouse_left = false;

    desktop->window_count = 0;
    desktop->focused = 0;
    desktop->dragging = 0;

    desktop->terminal_open = false;

    window_init(
        &terminal_window,
        "Terminal",
        width / 2 - 280,
        height / 2 - 170,
        560,
        340
    );

    terminal_window.visible = false;

    desktop->windows[0] = &terminal_window;
}

void desktop_mouse_move(
    desktop_t *desktop,
    int dx,
    int dy
) {
    if (!desktop) {
        return;
    }

    desktop->mouse_x += dx;
    desktop->mouse_y += dy;

    if (desktop->mouse_x < 0) {
        desktop->mouse_x = 0;
    }

    if (desktop->mouse_y < 0) {
        desktop->mouse_y = 0;
    }

    if (desktop->mouse_x >= desktop->width) {
        desktop->mouse_x = desktop->width - 1;
    }

    if (desktop->mouse_y >= desktop->height) {
        desktop->mouse_y = desktop->height - 1;
    }

    if (desktop->dragging) {
        window_move(
            desktop->dragging,
            desktop->mouse_x -
                desktop->dragging->drag_offset_x,
            desktop->mouse_y -
                desktop->dragging->drag_offset_y
        );
    }
}

void desktop_mouse_button(
    desktop_t *desktop,
    bool left
) {
    if (!desktop) {
        return;
    }

    desktop->mouse_left = left;

    /*
     * Only process the initial press.
     */
    if (!left || desktop->previous_mouse_left) {
        return;
    }

    /*
     * Terminal launcher.
     */
    if (desktop->mouse_y >= desktop->height - 48 &&
        desktop->mouse_x >= 12 &&
        desktop->mouse_x < 100) {

        terminal_window.visible = true;
        terminal_window.focused = true;

        desktop->focused = &terminal_window;
        desktop->terminal_open = true;

        return;
    }

    /*
     * Check windows from front to back.
     */
    {
        int i;

        for (i = DESKTOP_MAX_WINDOWS - 1; i >= 0; --i) {
            window_t *window = desktop->windows[i];

            if (!window || !window->visible) {
                continue;
            }

            if (!window_contains(
                    window,
                    desktop->mouse_x,
                    desktop->mouse_y)) {
                continue;
            }

            desktop->focused = window;
            window->focused = true;

            if (window_titlebar_contains(
                    window,
                    desktop->mouse_x,
                    desktop->mouse_y)) {

                window->dragging = true;

                window->drag_offset_x =
                    desktop->mouse_x - window->x;

                window->drag_offset_y =
                    desktop->mouse_y - window->y;

                desktop->dragging = window;
            }

            break;
        }
    }
}

void desktop_update(
    desktop_t *desktop
) {
    if (!desktop) {
        return;
    }

    /*
     * Release drag state when the button is released.
     */
    if (!desktop->mouse_left) {
        if (desktop->dragging) {
            desktop->dragging->dragging = false;
        }

        desktop->dragging = 0;
    }

    desktop->previous_mouse_left =
        desktop->mouse_left;
}

void desktop_render(
    desktop_t *desktop
) {
    int i;

    if (!desktop) {
        return;
    }

    /*
     * Background.
     */
    fill_rect(
        0,
        0,
        desktop->width,
        desktop->height,
        COLOR_DESKTOP
    );

    /*
     * Simple desktop accent.
     */
    fill_rect(
        0,
        0,
        desktop->width,
        3,
        0x00d0a050
    );

    /*
     * Taskbar.
     */
    fill_rect(
        0,
        desktop->height - 48,
        desktop->width,
        48,
        COLOR_TASKBAR
    );

    /*
     * Start button.
     */
    fill_rect(
        12,
        desktop->height - 38,
        72,
        28,
        0x00233343
    );

    /*
     * Terminal icon.
     */
    fill_rect(
        20,
        desktop->height - 32,
        22,
        16,
        0x000b1016
    );

    fill_rect(
        23,
        desktop->height - 29,
        5,
        2,
        COLOR_TEXT
    );

    /*
     * Windows.
     */
    for (i = 0; i < DESKTOP_MAX_WINDOWS; ++i) {
        if (desktop->windows[i]) {
            render_window(desktop->windows[i]);
        }
    }

    /*
     * Cursor is always rendered last.
     */
    draw_cursor(
        desktop->mouse_x,
        desktop->mouse_y
    );
}
