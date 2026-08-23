#include "desktop/desktop.h"

#include "desktop/window.h"
#include "graphics.h"
#include "mouse.h"

#define COLOR_DESKTOP  0x00101824
#define COLOR_TASKBAR  0x00151D28
#define COLOR_BORDER   0x003C5065
#define COLOR_TITLE    0x00212D3A
#define COLOR_TEXT     0x00D8E4EF
#define COLOR_CURSOR   0x00FFFFFF
#define COLOR_BLACK    0x00000000

static window_t terminal_window;

static void fill_rect(
    int x,
    int y,
    int width,
    int height,
    uint32_t color
)
{
    graphics_fill_rect(
        x,
        y,
        width,
        height,
        color
    );
}

static void draw_cursor(
    int x,
    int y
)
{
    static const uint8_t cursor[40] = {
        1, 1, 0, 0, 0,
        1, 1, 1, 0, 0,
        1, 1, 1, 1, 0,
        1, 1, 1, 1, 1,
        1, 1, 1, 1, 1,
        1, 1, 1, 0, 0,
        1, 1, 0, 0, 0,
        1, 0, 0, 0, 0
    };

    for (int row = 0;
         row < 8;
         row++) {

        for (int column = 0;
             column < 5;
             column++) {

            if (cursor[
                    row * 5 +
                    column
                ]) {

                graphics_put_pixel(
                    x + column,
                    y + row,
                    COLOR_CURSOR
                );
            }
        }
    }
}

static void render_window(
    const window_t* window
)
{
    if (!window ||
        !window->visible) {

        return;
    }

    if (window->width < 4 ||
        window->height < 26) {

        return;
    }

    /*
     * Window border.
     */
    fill_rect(
        window->x,
        window->y,
        window->width,
        window->height,
        window->border
    );

    /*
     * Window contents.
     */
    fill_rect(
        window->x + 1,
        window->y + 1,
        window->width - 2,
        window->height - 2,
        window->background
    );

    /*
     * Title bar.
     */
    fill_rect(
        window->x + 1,
        window->y + 1,
        window->width - 2,
        22,
        window->focused
            ? 0x002B3D50
            : window->titlebar
    );

    /*
     * Minimize button.
     */
    fill_rect(
        window->x +
            window->width - 54,
        window->y + 7,
        8,
        8,
        0x00D0A050
    );

    /*
     * Close button.
     */
    fill_rect(
        window->x +
            window->width - 34,
        window->y + 7,
        8,
        8,
        0x00D05050
    );
}

void desktop_init(
    desktop_t* desktop,
    int width,
    int height
)
{
    if (!desktop) {
        return;
    }

    desktop->width =
        width;

    desktop->height =
        height;

    desktop->mouse_x =
        width / 2;

    desktop->mouse_y =
        height / 2;

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

    window_init(
        &terminal_window,
        "Terminal",
        width / 2 - 280,
        height / 2 - 170,
        560,
        340
    );

    terminal_window.visible =
        false;

    desktop->windows[0] =
        &terminal_window;
}

void desktop_mouse_move(
    desktop_t* desktop,
    int dx,
    int dy
)
{
    if (!desktop) {
        return;
    }

    desktop->mouse_x +=
        dx;

    desktop->mouse_y +=
        dy;

    if (desktop->mouse_x < 0) {
        desktop->mouse_x = 0;
    }

    if (desktop->mouse_y < 0) {
        desktop->mouse_y = 0;
    }

    if (desktop->mouse_x >=
        desktop->width) {

        desktop->mouse_x =
            desktop->width - 1;
    }

    if (desktop->mouse_y >=
        desktop->height) {

        desktop->mouse_y =
            desktop->height - 1;
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
    desktop_t* desktop,
    bool left
)
{
    if (!desktop) {
        return;
    }

    desktop->mouse_left =
        left;

    if (!left ||
        desktop->previous_mouse_left) {

        return;
    }

    /*
     * Terminal launcher.
     */
    if (desktop->mouse_y >=
            desktop->height - 48 &&
        desktop->mouse_x >= 12 &&
        desktop->mouse_x < 100) {

        terminal_window.visible =
            true;

        terminal_window.focused =
            true;

        desktop->focused =
            &terminal_window;

        desktop->terminal_open =
            true;

        return;
    }

    /*
     * Search windows from front to back.
     */
    for (int i =
             DESKTOP_MAX_WINDOWS - 1;
         i >= 0;
         i--) {

        window_t* window =
            desktop->windows[i];

        if (!window ||
            !window->visible) {

            continue;
        }

        if (!window_contains(
                window,
                desktop->mouse_x,
                desktop->mouse_y
            )) {

            continue;
        }

        /*
         * Remove focus from every other
         * window.
         */
        for (int j = 0;
             j < DESKTOP_MAX_WINDOWS;
             j++) {

            if (desktop->windows[j]) {
                desktop->windows[j]->focused =
                    false;
            }
        }

        desktop->focused =
            window;

        window->focused =
            true;

        if (window_titlebar_contains(
                window,
                desktop->mouse_x,
                desktop->mouse_y
            )) {

            window->dragging =
                true;

            window->drag_offset_x =
                desktop->mouse_x -
                window->x;

            window->drag_offset_y =
                desktop->mouse_y -
                window->y;

            desktop->dragging =
                window;
        }

        break;
    }
}

void desktop_update(
    desktop_t* desktop
)
{
    if (!desktop) {
        return;
    }

    /*
     * Consume mouse events produced by IRQ12.
     */
    if (mouse_event_available()) {
        const struct mouse_state* mouse =
            mouse_get_state();

        if (mouse != 0) {
            if (mouse->delta_x != 0 ||
                mouse->delta_y != 0) {

                desktop_mouse_move(
                    desktop,
                    mouse->delta_x,
                    mouse->delta_y
                );
            }

            bool left =
                (mouse->buttons &
                 MOUSE_BUTTON_LEFT) != 0;

            if (left !=
                desktop->mouse_left) {

                desktop_mouse_button(
                    desktop,
                    left
                );
            }
        }

        mouse_clear_event();
    }

    /*
     * Release drag state.
     */
    if (!desktop->mouse_left) {
        if (desktop->dragging) {
            desktop->dragging->dragging =
                false;
        }

        desktop->dragging =
            0;
    }

    desktop->previous_mouse_left =
        desktop->mouse_left;
}

void desktop_render(
    desktop_t* desktop
)
{
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
     * Top accent.
     */
    fill_rect(
        0,
        0,
        desktop->width,
        3,
        0x00D0A050
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
     * Start / terminal button.
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
        0x000B1016
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
    for (int i = 0;
         i < DESKTOP_MAX_WINDOWS;
         i++) {

        if (desktop->windows[i]) {
            render_window(
                desktop->windows[i]
            );
        }
    }

    /*
     * Cursor is always drawn last.
     */
    draw_cursor(
        desktop->mouse_x,
        desktop->mouse_y
    );
}
