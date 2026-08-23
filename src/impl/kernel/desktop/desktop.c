#include "desktop/desktop.h"

#include "graphics.h"
#include "mouse.h"

#define COLOR_DESKTOP       0x17172A
#define COLOR_TASKBAR       0x24243A
#define COLOR_WINDOW        0xD8D8E0
#define COLOR_BORDER        0xFFFFFF
#define COLOR_TITLEBAR      0x1769AA
#define COLOR_TITLEBAR_DARK 0x0E416B
#define COLOR_TEXT          0xFFFFFF
#define COLOR_CURSOR        0xFFFFFF
#define COLOR_CURSOR_EDGE   0x000000
#define COLOR_CONTENT       0xE2E2EA
#define COLOR_CONTENT_EDGE  0x9A9AAA

#define TASKBAR_HEIGHT 40
#define TOPBAR_HEIGHT 42

static window_t test_window;

static void draw_cursor(
    int x,
    int y
)
{
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

            if (cursor[row][col] == 'X') {

                graphics_put_pixel(
                    x + col,
                    y + row,
                    COLOR_CURSOR_EDGE
                );
            }
        }
    }

    static const char interior[16][12] = {
        "            ",
        "X           ",
        "X           ",
        "X.X         ",
        "X...X       ",
        "X....X      ",
        "X.....X     ",
        "X......X    ",
        "X.......X   ",
        "X........X  ",
        "X.........X ",
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

            if (interior[row][col] == 'X') {

                graphics_put_pixel(
                    x + col,
                    y + row,
                    COLOR_CURSOR
                );
            }
        }
    }
}

static void draw_window(
    window_t *window
)
{
    if (window == 0 ||
        !window->visible) {

        return;
    }

    graphics_fill_rect(
        window->x + 5,
        window->y + 5,
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

    uint32_t title_color =
        window->focused
            ? COLOR_TITLEBAR
            : 0x45455A;

    graphics_fill_rect(
        window->x,
        window->y,
        window->width,
        WINDOW_TITLEBAR_HEIGHT,
        title_color
    );

    graphics_fill_rect(
        window->x,
        window->y +
            WINDOW_TITLEBAR_HEIGHT - 1,
        window->width,
        1,
        COLOR_TITLEBAR_DARK
    );

    graphics_draw_text(
        window->x + 10,
        window->y + 8,
        window->title,
        COLOR_TEXT,
        title_color,
        2
    );

    int close_x =
        window->x +
        window->width -
        24;

    graphics_fill_rect(
        close_x,
        window->y + 6,
        16,
        16,
        0xD83A56
    );

    graphics_draw_text(
        close_x + 3,
        window->y + 9,
        "X",
        COLOR_TEXT,
        0xD83A56,
        1
    );

    int content_x =
        window->x + 20;

    int content_y =
        window->y + 55;

    int content_width =
        window->width - 40;

    int content_height =
        window->height - 75;

    if (content_width <= 0 ||
        content_height <= 0) {

        return;
    }

    graphics_fill_rect(
        content_x,
        content_y,
        content_width,
        content_height,
        COLOR_CONTENT
    );

    graphics_rect(
        content_x,
        content_y,
        content_width,
        content_height,
        COLOR_CONTENT_EDGE
    );

    graphics_draw_text(
        content_x + 18,
        content_y + 23,
        "Welcome to FazbearOS",
        0x101020,
        COLOR_CONTENT,
        2
    );

    graphics_draw_text(
        content_x + 18,
        content_y + 53,
        "Desktop is running.",
        0x101020,
        COLOR_CONTENT,
        1
    );

    graphics_draw_text(
        content_x + 18,
        content_y + 75,
        "Move the mouse.",
        0x101020,
        COLOR_CONTENT,
        1
    );

    graphics_draw_text(
        content_x + 18,
        content_y + 97,
        "Drag this title bar.",
        0x101020,
        COLOR_CONTENT,
        1
    );

    graphics_draw_text(
        content_x + 18,
        content_y + 119,
        "Click the window to focus it.",
        0x101020,
        COLOR_CONTENT,
        1
    );
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

    if (width < 1) {
        width = 1;
    }

    if (height < 1) {
        height = 1;
    }

    desktop->width =
        width;

    desktop->height =
        height;

    /*
     * Tell the mouse driver the actual
     * framebuffer dimensions.
     */
    mouse_set_screen_size(
        width,
        height
    );

    /*
     * Start the cursor at the center of
     * the actual display.
     */
    desktop->mouse_x =
        width / 2;

    desktop->mouse_y =
        height / 2;

    /*
     * Keep the driver's position synchronized
     * with the desktop cursor.
     *
     * The mouse state itself will be updated
     * by subsequent movement packets.
     */
    const struct mouse_state*
        mouse =
        mouse_get_state();

    if (mouse != 0) {

        /*
         * The driver starts at zero. Move the
         * visible cursor to the center without
         * generating a fake mouse event.
         */
        int center_x =
            width / 2;

        int center_y =
            height / 2;

        /*
         * There is no setter for the driver's
         * position because normal operation
         * should be driven by hardware movement.
         *
         * Start the desktop at the driver's
         * actual position instead so both layers
         * remain synchronized.
         */
        desktop->mouse_x =
            mouse->x;

        desktop->mouse_y =
            mouse->y;

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

        (void)center_x;
        (void)center_y;
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

    int window_width =
        500;

    int window_height =
        300;

    if (window_width >
        width - 20) {

        window_width =
            width - 20;
    }

    if (window_height >
        height - 80) {

        window_height =
            height - 80;
    }

    if (window_width < 120) {
        window_width =
            width;
    }

    if (window_height < 80) {
        window_height =
            height;
    }

    int window_x =
        (width - window_width) / 2;

    int window_y =
        (height - window_height) / 2;

    if (window_y <
        TOPBAR_HEIGHT + 4) {

        window_y =
            TOPBAR_HEIGHT + 4;
    }

    window_init(
        &test_window,
        "FazbearOS",
        window_x,
        window_y,
        window_width,
        window_height
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

    /*
     * Poll the PS/2 controller every frame.
     *
     * IRQ12 will also call mouse_interrupt()
     * when interrupts are working. The driver
     * safely handles both paths.
     */
    mouse_poll();

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

void desktop_render(
    desktop_t *desktop
)
{
    if (desktop == 0 ||
        !graphics_available()) {

        return;
    }

    graphics_clear(
        COLOR_DESKTOP
    );

    graphics_fill_rect(
        0,
        0,
        desktop->width,
        TOPBAR_HEIGHT,
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

    int taskbar_y =
        desktop->height -
        TASKBAR_HEIGHT;

    if (taskbar_y < 0) {
        taskbar_y = 0;
    }

    graphics_fill_rect(
        0,
        taskbar_y,
        desktop->width,
        TASKBAR_HEIGHT,
        COLOR_TASKBAR
    );

    graphics_fill_rect(
        0,
        taskbar_y,
        desktop->width,
        2,
        0x00AAFF
    );

    int start_width =
        100;

    if (start_width >
        desktop->width - 24) {

        start_width =
            desktop->width - 24;
    }

    if (start_width > 0) {

        graphics_fill_rect(
            12,
            taskbar_y + 8,
            start_width,
            24,
            COLOR_TITLEBAR
        );

        graphics_draw_text(
            26,
            taskbar_y + 14,
            "START",
            COLOR_TEXT,
            COLOR_TITLEBAR,
            1
        );
    }

    for (int i = 0;
         i < desktop->window_count;
         i++) {

        draw_window(
            desktop->windows[i]
        );
    }

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

    /*
     * Mouse-down transition.
     */
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

    /*
     * Mouse-up transition.
     */
    if (!left &&
        desktop->previous_mouse_left) {

        if (desktop->dragging != 0) {

            desktop->dragging->dragging =
                false;

            desktop->dragging =
                0;
        }
    }

    desktop->mouse_left =
        left;

    desktop->previous_mouse_left =
        left;
}
