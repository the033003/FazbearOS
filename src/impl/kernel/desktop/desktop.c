#include "desktop/desktop.h"

#include "graphics.h"
#include "mouse.h"
#include "keyboard.h"
#include "rtc.h"
#include "io.h"

#define COLOR_DESKTOP       0x0E1118
#define COLOR_DESKTOP_TOP   0x151A24
#define COLOR_TASKBAR       0x181E29
#define COLOR_TASKBAR_TOP   0x2D394B
#define COLOR_START         0x253247
#define COLOR_START_HOVER   0x31415C
#define COLOR_WINDOW        0xE7E7EF
#define COLOR_BORDER        0x596070
#define COLOR_TITLEBAR      0x273247
#define COLOR_TITLE_ACTIVE  0x31577D
#define COLOR_TITLE_DARK    0x192231
#define COLOR_TEXT          0xFFFFFF
#define COLOR_TEXT_DARK     0x111722
#define COLOR_ACCENT        0x55D6BE
#define COLOR_DANGER        0xD94B5B
#define COLOR_WARNING       0xE7B84B
#define COLOR_MUTED         0x8290A8

#define START_MENU_HOVER    COLOR_START_HOVER

#define DESKTOP_TOPBAR_HEIGHT 34
#define DESKTOP_TASKBAR_HEIGHT 42

#define START_BUTTON_X 10
#define START_BUTTON_WIDTH 88

#define START_MENU_WIDTH 250
#define START_MENU_HEIGHT 330

#define NIBBLE_BUTTON_Y 58
#define NIBBLE_BUTTON_HEIGHT 48

#define TERMINAL_BUTTON_Y 112
#define TERMINAL_BUTTON_HEIGHT 48

#define START_ACTION_Y 250
#define START_ACTION_HEIGHT 28
#define START_ACTION_GAP 6

static void desktop_restart(void)
{
    for (
        int i = 0;
        i < 100000;
        i++
    ) {
        if (
            (inb(0x64) & 0x02) == 0
        ) {
            break;
        }
    }

    outb(
        0x64,
        0xFE
    );

    for (;;) {
        __asm__ volatile ("hlt");
    }
}

static void desktop_power_off(void)
{
    outw(
        0x604,
        0x2000
    );

    outw(
        0xB004,
        0x2000
    );

    for (;;) {
        __asm__ volatile ("hlt");
    }
}

static void desktop_draw_power_icon(
    int x,
    int y,
    uint32_t color,
    uint32_t background
)
{
    graphics_draw_text(
        x,
        y,
        "P",
        color,
        background,
        1
    );
}

static void desktop_draw_restart_icon(
    int x,
    int y,
    uint32_t color,
    uint32_t background
)
{
    graphics_draw_text(
        x,
        y,
        "R",
        color,
        background,
        1
    );
}

static window_t *desktop_top_window(
    desktop_t *desktop,
    int x,
    int y
)
{
    if (desktop == 0) {
        return 0;
    }

    for (
        int i =
            desktop->window_count - 1;
        i >= 0;
        i--
    ) {
        window_t *window =
            desktop->windows[i];

        if (
            window != 0 &&
            window->visible &&
            !window->minimized &&
            window_contains(
                window,
                x,
                y
            )
        ) {
            return window;
        }
    }

    return 0;
}

static void desktop_bring_to_front(
    desktop_t *desktop,
    window_t *window
)
{
    if (
        desktop == 0 ||
        window == 0
    ) {
        return;
    }

    int index = -1;

    for (
        int i = 0;
        i < desktop->window_count;
        i++
    ) {
        if (
            desktop->windows[i] ==
            window
        ) {
            index = i;
            break;
        }
    }

    if (index < 0) {
        return;
    }

    for (
        int i = index;
        i < desktop->window_count - 1;
        i++
    ) {
        desktop->windows[i] =
            desktop->windows[i + 1];
    }

    desktop->windows[
        desktop->window_count - 1
    ] = window;
}

static void desktop_focus(
    desktop_t *desktop,
    window_t *window
)
{
    if (desktop == 0) {
        return;
    }

    desktop->focused =
        window;

    for (
        int i = 0;
        i < desktop->window_count;
        i++
    ) {
        if (
            desktop->windows[i] != 0
        ) {
            desktop->windows[i]->focused =
                desktop->windows[i] ==
                window;
        }
    }

    if (window != 0) {
        desktop_bring_to_front(
            desktop,
            window
        );
    }
}

static void desktop_focus_next(
    desktop_t *desktop
)
{
    if (desktop == 0) {
        return;
    }

    desktop->focused = 0;

    for (
        int i =
            desktop->window_count - 1;
        i >= 0;
        i--
    ) {
        window_t *window =
            desktop->windows[i];

        if (
            window != 0 &&
            window->visible &&
            !window->minimized
        ) {
            desktop_focus(
                desktop,
                window
            );

            return;
        }
    }

    for (
        int i = 0;
        i < desktop->window_count;
        i++
    ) {
        if (
            desktop->windows[i] != 0
        ) {
            desktop->windows[i]->focused =
                false;
        }
    }
}

static void desktop_clamp_window(
    const desktop_t *desktop,
    window_t *window
)
{
    if (
        desktop == 0 ||
        window == 0
    ) {
        return;
    }

    int top_limit =
        DESKTOP_TOPBAR_HEIGHT;

    int bottom_limit =
        desktop->height -
        DESKTOP_TASKBAR_HEIGHT;

    int max_x =
        desktop->width -
        window->width;

    int max_y =
        bottom_limit -
        window->height;

    if (max_x < 0) {
        max_x = 0;
    }

    if (max_y < top_limit) {
        max_y = top_limit;
    }

    if (window->x < 0) {
        window->x = 0;
    }

    if (window->y < top_limit) {
        window->y =
            top_limit;
    }

    if (window->x > max_x) {
        window->x =
            max_x;
    }

    if (window->y > max_y) {
        window->y =
            max_y;
    }
}

static bool point_in_rect(
    int x,
    int y,
    int rx,
    int ry,
    int width,
    int height
)
{
    return
        x >= rx &&
        x < rx + width &&
        y >= ry &&
        y < ry + height;
}

static bool close_button_contains(
    const window_t *window,
    int x,
    int y
)
{
    if (
        window == 0 ||
        !window->closable
    ) {
        return false;
    }

    return point_in_rect(
        x,
        y,
        window->x +
            window->width -
            24,
        window->y + 6,
        16,
        16
    );
}

static bool minimize_button_contains(
    const window_t *window,
    int x,
    int y
)
{
    if (window == 0) {
        return false;
    }

    return point_in_rect(
        x,
        y,
        window->x +
            window->width -
            47,
        window->y + 6,
        16,
        16
    );
}

static void desktop_add_window(
    desktop_t *desktop,
    window_t *window
)
{
    if (
        desktop == 0 ||
        window == 0
    ) {
        return;
    }

    if (
        desktop->window_count >=
        DESKTOP_MAX_WINDOWS
    ) {
        return;
    }

    desktop->windows[
        desktop->window_count
    ] = window;

    desktop->window_count++;
}

static void draw_cursor(
    int x,
    int y
)
{
    static const char cursor[16][13] = {
        "X            ",
        "XX           ",
        "X.X          ",
        "X..X         ",
        "X...X        ",
        "X....X       ",
        "X.....X      ",
        "X......X     ",
        "X.......X    ",
        "X........X   ",
        "X.........X  ",
        "X......XXXX  ",
        "X...X        ",
        "X..XX        ",
        "X.X          ",
        "XX           "
    };

    static const char interior[16][13] = {
        "             ",
        "X            ",
        "X            ",
        "X.X          ",
        "X..X         ",
        "X...X        ",
        "X....X       ",
        "X.....X      ",
        "X......X     ",
        "X.......X    ",
        "X........X   ",
        "X......XX    ",
        "X...X        ",
        "X..X         ",
        "X.X          ",
        "             "
    };

    for (
        int row = 0;
        row < 16;
        row++
    ) {
        for (
            int col = 0;
            col < 12;
            col++
        ) {
            if (
                cursor[row][col] ==
                'X'
            ) {
                graphics_put_pixel(
                    x + col,
                    y + row,
                    0x000000
                );
            }
        }
    }

    for (
        int row = 0;
        row < 16;
        row++
    ) {
        for (
            int col = 0;
            col < 12;
            col++
        ) {
            if (
                interior[row][col] ==
                'X'
            ) {
                graphics_put_pixel(
                    x + col,
                    y + row,
                    0xFFFFFF
                );
            }
        }
    }
}

static void draw_window(
    const window_t *window,
    const desktop_t *desktop
)
{
    if (
        window == 0 ||
        desktop == 0 ||
        !window->visible ||
        window->minimized
    ) {
        return;
    }

    uint32_t title_color =
        window->focused
            ? COLOR_TITLE_ACTIVE
            : COLOR_TITLEBAR;

    graphics_fill_rect(
        window->x + 5,
        window->y + 5,
        window->width,
        window->height,
        0x070A0F
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
        window->focused
            ? COLOR_ACCENT
            : window->border
    );

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
            WINDOW_TITLEBAR_HEIGHT -
            1,
        window->width,
        1,
        COLOR_TITLE_DARK
    );

    graphics_draw_text(
        window->x + 10,
        window->y + 8,
        window->title,
        COLOR_TEXT,
        title_color,
        1
    );

    if (window->closable) {
        graphics_fill_rect(
            window->x +
                window->width -
                24,
            window->y + 6,
            16,
            16,
            COLOR_DANGER
        );

        graphics_draw_text(
            window->x +
                window->width -
                21,
            window->y + 9,
            "X",
            COLOR_TEXT,
            COLOR_DANGER,
            1
        );
    }

    graphics_fill_rect(
        window->x +
            window->width -
            47,
        window->y + 6,
        16,
        16,
        0x46536A
    );

    graphics_draw_text(
        window->x +
            window->width -
            44,
        window->y + 8,
        "-",
        COLOR_TEXT,
        0x46536A,
        1
    );

    if (
        window->app_id ==
        WINDOW_APP_NIBBLE
    ) {
        nibble_render(
            &desktop->nibble,
            window
        );

        return;
    }

    if (
        window->app_id ==
        WINDOW_APP_TERMINAL
    ) {
        terminal_render(
            &desktop->terminal,
            window
        );

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
        window->x + 12,
        window->y + 40,
        content_width,
        content_height,
        COLOR_WINDOW
    );

    graphics_draw_text(
        window->x + 25,
        window->y + 58,
        "FazbearOS desktop",
        COLOR_TEXT_DARK,
        COLOR_WINDOW,
        1
    );

    graphics_draw_text(
        window->x + 25,
        window->y + 76,
        "Welcome to your new desktop.",
        COLOR_TEXT_DARK,
        COLOR_WINDOW,
        1
    );
}

static void draw_top_bar(
    const desktop_t *desktop
)
{
    graphics_fill_rect(
        0,
        0,
        desktop->width,
        DESKTOP_TOPBAR_HEIGHT,
        COLOR_DESKTOP_TOP
    );

    graphics_fill_rect(
        0,
        DESKTOP_TOPBAR_HEIGHT - 1,
        desktop->width,
        1,
        COLOR_TASKBAR_TOP
    );

    graphics_draw_text(
        14,
        9,
        "FAZBEAROS",
        COLOR_ACCENT,
        COLOR_DESKTOP_TOP,
        2
    );

    graphics_draw_text(
        122,
        11,
        "DESKTOP",
        COLOR_MUTED,
        COLOR_DESKTOP_TOP,
        1
    );
}

static void draw_clock(
    const desktop_t *desktop
)
{
    struct rtc_datetime datetime;

    rtc_read(
        &datetime
    );

    char clock[6];

    clock[0] =
        (char)(
            '0' +
            (datetime.hour / 10)
        );

    clock[1] =
        (char)(
            '0' +
            (datetime.hour % 10)
        );

    clock[2] = ':';

    clock[3] =
        (char)(
            '0' +
            (datetime.minute / 10)
        );

    clock[4] =
        (char)(
            '0' +
            (datetime.minute % 10)
        );

    clock[5] = '\0';

    graphics_draw_text(
        desktop->width - 50,
        desktop->height - 25,
        clock,
        COLOR_TEXT,
        COLOR_TASKBAR,
        1
    );
}

static void draw_taskbar(
    const desktop_t *desktop
)
{
    int y =
        desktop->height -
        DESKTOP_TASKBAR_HEIGHT;

    graphics_fill_rect(
        0,
        y,
        desktop->width,
        DESKTOP_TASKBAR_HEIGHT,
        COLOR_TASKBAR
    );

    graphics_fill_rect(
        0,
        y,
        desktop->width,
        2,
        COLOR_TASKBAR_TOP
    );

    uint32_t start_color =
        desktop->start_menu_open
            ? START_MENU_HOVER
            : COLOR_START;

    graphics_fill_rect(
        START_BUTTON_X,
        y + 7,
        START_BUTTON_WIDTH,
        28,
        start_color
    );

    graphics_draw_text(
        START_BUTTON_X + 14,
        y + 17,
        "START",
        COLOR_TEXT,
        start_color,
        1
    );

    int task_x =
        START_BUTTON_X +
        START_BUTTON_WIDTH +
        8;

    for (
        int i = 0;
        i < desktop->window_count;
        i++
    ) {
        const window_t *window =
            desktop->windows[i];

        if (
            window == 0 ||
            !window->visible
        ) {
            continue;
        }

        int width = 92;

        uint32_t color =
            window->focused &&
            !window->minimized
                ? COLOR_TITLE_ACTIVE
                : COLOR_START;

        graphics_fill_rect(
            task_x,
            y + 7,
            width,
            28,
            color
        );

        graphics_draw_text(
            task_x + 8,
            y + 17,
            window->title,
            COLOR_TEXT,
            color,
            1
        );

        task_x +=
            width + 5;

        if (
            task_x >
            desktop->width - 120
        ) {
            break;
        }
    }

    graphics_draw_text(
        desktop->width - 105,
        y + 17,
        "READY",
        COLOR_ACCENT,
        COLOR_TASKBAR,
        1
    );

    draw_clock(
        desktop
    );
}

static void draw_start_menu(
    const desktop_t *desktop
)
{
    if (
        !desktop->start_menu_open
    ) {
        return;
    }

    int x = 10;

    int y =
        desktop->height -
        DESKTOP_TASKBAR_HEIGHT -
        START_MENU_HEIGHT -
        8;

    graphics_fill_rect(
        x + 5,
        y + 5,
        START_MENU_WIDTH,
        START_MENU_HEIGHT,
        0x070A0F
    );

    graphics_fill_rect(
        x,
        y,
        START_MENU_WIDTH,
        START_MENU_HEIGHT,
        0x1A2130
    );

    graphics_rect(
        x,
        y,
        START_MENU_WIDTH,
        START_MENU_HEIGHT,
        COLOR_TASKBAR_TOP
    );

    graphics_fill_rect(
        x + 1,
        y + 1,
        START_MENU_WIDTH - 2,
        45,
        0x202A3C
    );

    graphics_draw_text(
        x + 16,
        y + 12,
        "FAZBEAROS",
        COLOR_ACCENT,
        0x202A3C,
        1
    );

    graphics_draw_text(
        x + 16,
        y + 27,
        "Welcome back",
        COLOR_MUTED,
        0x202A3C,
        1
    );

    graphics_draw_text(
        x + 16,
        y + 52,
        "APPLICATIONS",
        COLOR_MUTED,
        0x1A2130,
        1
    );

    /*
     * Nibble.
     */
    uint32_t nibble_color =
        point_in_rect(
            desktop->mouse_x,
            desktop->mouse_y,
            x + 10,
            y + NIBBLE_BUTTON_Y,
            START_MENU_WIDTH - 20,
            NIBBLE_BUTTON_HEIGHT
        )
            ? START_MENU_HOVER
            : COLOR_START;

    graphics_fill_rect(
        x + 10,
        y + NIBBLE_BUTTON_Y,
        START_MENU_WIDTH - 20,
        NIBBLE_BUTTON_HEIGHT,
        nibble_color
    );

    graphics_draw_text(
        x + 20,
        y + NIBBLE_BUTTON_Y + 10,
        "NIBBLE",
        COLOR_ACCENT,
        nibble_color,
        1
    );

    graphics_draw_text(
        x + 20,
        y + NIBBLE_BUTTON_Y + 25,
        "Small scratchpad",
        COLOR_MUTED,
        nibble_color,
        1
    );

    /*
     * Terminal.
     */
    uint32_t terminal_color =
        point_in_rect(
            desktop->mouse_x,
            desktop->mouse_y,
            x + 10,
            y + TERMINAL_BUTTON_Y,
            START_MENU_WIDTH - 20,
            TERMINAL_BUTTON_HEIGHT
        )
            ? START_MENU_HOVER
            : COLOR_START;

    graphics_fill_rect(
        x + 10,
        y + TERMINAL_BUTTON_Y,
        START_MENU_WIDTH - 20,
        TERMINAL_BUTTON_HEIGHT,
        terminal_color
    );

    graphics_draw_text(
        x + 20,
        y + TERMINAL_BUTTON_Y + 10,
        "TERMINAL",
        COLOR_ACCENT,
        terminal_color,
        1
    );

    graphics_draw_text(
        x + 20,
        y + TERMINAL_BUTTON_Y + 25,
        "Command shell",
        COLOR_MUTED,
        terminal_color,
        1
    );

    /*
     * Separator.
     */
    graphics_fill_rect(
        x + 12,
        y + 230,
        START_MENU_WIDTH - 24,
        1,
        COLOR_BORDER
    );

    /*
     * Restart.
     */
    uint32_t restart_color =
        point_in_rect(
            desktop->mouse_x,
            desktop->mouse_y,
            x + 10,
            y + START_ACTION_Y,
            START_MENU_WIDTH - 20,
            START_ACTION_HEIGHT
        )
            ? START_MENU_HOVER
            : 0x202A3C;

    graphics_fill_rect(
        x + 10,
        y + START_ACTION_Y,
        START_MENU_WIDTH - 20,
        START_ACTION_HEIGHT,
        restart_color
    );

    desktop_draw_restart_icon(
        x + 20,
        y + START_ACTION_Y + 10,
        COLOR_WARNING,
        restart_color
    );

    graphics_draw_text(
        x + 42,
        y + START_ACTION_Y + 10,
        "Restart",
        COLOR_TEXT,
        restart_color,
        1
    );

    /*
     * Power off.
     */
    int power_y =
        START_ACTION_Y +
        START_ACTION_HEIGHT +
        START_ACTION_GAP;

    uint32_t power_color =
        point_in_rect(
            desktop->mouse_x,
            desktop->mouse_y,
            x + 10,
            y + power_y,
            START_MENU_WIDTH - 20,
            START_ACTION_HEIGHT
        )
            ? 0x5A2934
            : 0x202A3C;

    graphics_fill_rect(
        x + 10,
        y + power_y,
        START_MENU_WIDTH - 20,
        START_ACTION_HEIGHT,
        power_color
    );

    desktop_draw_power_icon(
        x + 20,
        y + power_y + 10,
        COLOR_DANGER,
        power_color
    );

    graphics_draw_text(
        x + 42,
        y + power_y + 10,
        "Power off",
        COLOR_TEXT,
        power_color,
        1
    );

    graphics_draw_text(
        x + 16,
        y + 308,
        "FazbearOS",
        COLOR_MUTED,
        0x1A2130,
        1
    );
}

static void draw_desktop(
    const desktop_t *desktop
)
{
    graphics_clear(
        COLOR_DESKTOP
    );

    graphics_fill_rect(
        0,
        DESKTOP_TOPBAR_HEIGHT,
        desktop->width,
        desktop->height -
            DESKTOP_TOPBAR_HEIGHT -
            DESKTOP_TASKBAR_HEIGHT,
        COLOR_DESKTOP
    );

    for (
        int x = 0;
        x < desktop->width;
        x += 64
    ) {
        graphics_fill_rect(
            x,
            DESKTOP_TOPBAR_HEIGHT,
            1,
            desktop->height -
                DESKTOP_TOPBAR_HEIGHT -
                DESKTOP_TASKBAR_HEIGHT,
            0x111720
        );
    }

    for (
        int y = DESKTOP_TOPBAR_HEIGHT;
        y <
            desktop->height -
            DESKTOP_TASKBAR_HEIGHT;
        y += 64
    ) {
        graphics_fill_rect(
            0,
            y,
            desktop->width,
            1,
            0x111720
        );
    }

    draw_top_bar(
        desktop
    );

    for (
        int i = 0;
        i < desktop->window_count;
        i++
    ) {
        draw_window(
            desktop->windows[i],
            desktop
        );
    }

    draw_start_menu(
        desktop
    );

    draw_taskbar(
        desktop
    );

    draw_cursor(
        desktop->mouse_x,
        desktop->mouse_y
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

    if (width < 320) {
        width = 320;
    }

    if (height < 240) {
        height = 240;
    }

    desktop->width =
        width;

    desktop->height =
        height;

    mouse_set_screen_size(
        width,
        height
    );

    desktop->mouse_x =
        width / 2;

    desktop->mouse_y =
        height / 2;

    desktop->mouse_left =
        false;

    desktop->previous_mouse_left =
        false;

    desktop->start_menu_open =
        false;

    desktop->window_count =
        0;

    desktop->focused =
        0;

    desktop->dragging =
        0;

    for (
        int i = 0;
        i < DESKTOP_MAX_WINDOWS;
        i++
    ) {
        desktop->windows[i] =
            0;
    }

    /*
     * Nibble.
     */
    nibble_init(
        &desktop->nibble
    );

    int nibble_width =
        520;

    int nibble_height =
        330;

    if (
        nibble_width >
        width - 30
    ) {
        nibble_width =
            width - 30;
    }

    if (
        nibble_height >
        height -
        DESKTOP_TOPBAR_HEIGHT -
        DESKTOP_TASKBAR_HEIGHT -
        20
    ) {
        nibble_height =
            height -
            DESKTOP_TOPBAR_HEIGHT -
            DESKTOP_TASKBAR_HEIGHT -
            20;
    }

    if (nibble_width < 260) {
        nibble_width = 260;
    }

    if (nibble_height < 190) {
        nibble_height = 190;
    }

    window_init(
        &desktop->nibble_window,
        "Nibble",
        (width - nibble_width) / 2,
        (height - nibble_height) / 2,
        nibble_width,
        nibble_height
    );

    desktop->nibble_window.app_id =
        WINDOW_APP_NIBBLE;

    desktop->nibble_window.titlebar =
        0x31577D;

    /*
     * Nibble starts hidden. It is still registered with
     * the desktop so the Start Menu can launch it.
     */
    desktop->nibble_window.visible =
        false;

    desktop->nibble_window.minimized =
        false;

    desktop->nibble_window.focused =
        false;

    desktop_add_window(
        desktop,
        &desktop->nibble_window
    );

    /*
     * Terminal.
     */
    terminal_init(
        &desktop->terminal
    );

    int terminal_width =
        650;

    int terminal_height =
        420;

    if (
        terminal_width >
        width - 30
    ) {
        terminal_width =
            width - 30;
    }

    if (
        terminal_height >
        height -
        DESKTOP_TOPBAR_HEIGHT -
        DESKTOP_TASKBAR_HEIGHT -
        20
    ) {
        terminal_height =
            height -
            DESKTOP_TOPBAR_HEIGHT -
            DESKTOP_TASKBAR_HEIGHT -
            20;
    }

    if (terminal_width < 300) {
        terminal_width = 300;
    }

    if (terminal_height < 220) {
        terminal_height = 220;
    }

    window_init(
        &desktop->terminal_window,
        "Terminal",
        (width - terminal_width) / 2,
        (height - terminal_height) / 2,
        terminal_width,
        terminal_height
    );

    desktop->terminal_window.app_id =
        WINDOW_APP_TERMINAL;

    desktop->terminal_window.background =
        0x080C12;

    desktop->terminal_window.border =
        0x30415A;

    desktop->terminal_window.titlebar =
        0x172334;

    /*
     * Terminal also starts hidden.
     */
    desktop->terminal_window.visible =
        false;

    desktop->terminal_window.minimized =
        false;

    desktop->terminal_window.focused =
        false;

    desktop_add_window(
        desktop,
        &desktop->terminal_window
    );
}

void desktop_update(
    desktop_t *desktop
)
{
    if (desktop == 0) {
        return;
    }

    mouse_poll();

    const struct mouse_state *mouse =
        mouse_get_state();

    if (
        mouse != 0 &&
        mouse_event_available()
    ) {
        desktop_mouse_move(
            desktop,
            mouse->delta_x,
            mouse->delta_y
        );

        bool left =
            (
                mouse->buttons &
                MOUSE_BUTTON_LEFT
            ) != 0;

        desktop_mouse_button(
            desktop,
            left
        );

        mouse_clear_event();
    }

    /*
     * The focused graphical application owns the keyboard.
     */
    if (
        desktop->focused != 0 &&
        desktop->focused->visible &&
        !desktop->focused->minimized
    ) {
        if (
            desktop->focused->app_id ==
            WINDOW_APP_NIBBLE
        ) {
            nibble_update(
                &desktop->nibble
            );
        } else if (
            desktop->focused->app_id ==
            WINDOW_APP_TERMINAL
        ) {
            terminal_update(
                &desktop->terminal
            );
        }
    }
}

void desktop_render(
    desktop_t *desktop
)
{
    if (
        desktop == 0 ||
        !graphics_available()
    ) {
        return;
    }

    draw_desktop(
        desktop
    );

    graphics_present();
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

    if (
        desktop->mouse_x < 0
    ) {
        desktop->mouse_x = 0;
    }

    if (
        desktop->mouse_y < 0
    ) {
        desktop->mouse_y = 0;
    }

    if (
        desktop->mouse_x >=
        desktop->width
    ) {
        desktop->mouse_x =
            desktop->width - 1;
    }

    if (
        desktop->mouse_y >=
        desktop->height
    ) {
        desktop->mouse_y =
            desktop->height - 1;
    }

    if (
        desktop->dragging != 0
    ) {
        window_t *window =
            desktop->dragging;

        window_move(
            window,
            desktop->mouse_x -
                window->drag_offset_x,
            desktop->mouse_y -
                window->drag_offset_y
        );

        desktop_clamp_window(
            desktop,
            window
        );
    }
}

static void desktop_taskbar_click(
    desktop_t *desktop
)
{
    int taskbar_y =
        desktop->height -
        DESKTOP_TASKBAR_HEIGHT;

    if (
        desktop->mouse_y <
        taskbar_y
    ) {
        return;
    }

    /*
     * Start button.
     */
    if (
        point_in_rect(
            desktop->mouse_x,
            desktop->mouse_y,
            START_BUTTON_X,
            taskbar_y + 7,
            START_BUTTON_WIDTH,
            28
        )
    ) {
        desktop->start_menu_open =
            !desktop->start_menu_open;

        desktop->dragging =
            0;

        return;
    }

    /*
     * Application taskbar buttons.
     */
    int task_x =
        START_BUTTON_X +
        START_BUTTON_WIDTH +
        8;

    for (
        int i = 0;
        i < desktop->window_count;
        i++
    ) {
        window_t *window =
            desktop->windows[i];

        if (
            window == 0 ||
            !window->visible
        ) {
            continue;
        }

        if (
            point_in_rect(
                desktop->mouse_x,
                desktop->mouse_y,
                task_x,
                taskbar_y + 7,
                92,
                28
            )
        ) {
            if (
                window->minimized ||
                desktop->focused != window
            ) {
                window->minimized =
                    false;

                desktop_focus(
                    desktop,
                    window
                );
            } else {
                window->minimized =
                    true;

                window->focused =
                    false;

                if (
                    desktop->focused ==
                    window
                ) {
                    desktop->focused =
                        0;

                    desktop_focus_next(
                        desktop
                    );
                }
            }

            return;
        }

        task_x += 97;

        if (
            task_x >
            desktop->width - 120
        ) {
            break;
        }
    }
}

static bool desktop_start_menu_click(
    desktop_t *desktop
)
{
    if (
        desktop == 0 ||
        !desktop->start_menu_open
    ) {
        return false;
    }

    int x = 10;

    int y =
        desktop->height -
        DESKTOP_TASKBAR_HEIGHT -
        START_MENU_HEIGHT -
        8;

    /*
     * Nibble.
     */
    if (
        point_in_rect(
            desktop->mouse_x,
            desktop->mouse_y,
            x + 10,
            y + NIBBLE_BUTTON_Y,
            START_MENU_WIDTH - 20,
            NIBBLE_BUTTON_HEIGHT
        )
    ) {
        desktop->nibble_window.visible =
            true;

        desktop->nibble_window.minimized =
            false;

        desktop_focus(
            desktop,
            &desktop->nibble_window
        );

        desktop_clamp_window(
            desktop,
            &desktop->nibble_window
        );

        desktop->start_menu_open =
            false;

        return true;
    }

    /*
     * Terminal.
     */
    if (
        point_in_rect(
            desktop->mouse_x,
            desktop->mouse_y,
            x + 10,
            y + TERMINAL_BUTTON_Y,
            START_MENU_WIDTH - 20,
            TERMINAL_BUTTON_HEIGHT
        )
    ) {
        desktop->terminal_window.visible =
            true;

        desktop->terminal_window.minimized =
            false;

        desktop_focus(
            desktop,
            &desktop->terminal_window
        );

        desktop_clamp_window(
            desktop,
            &desktop->terminal_window
        );

        desktop->start_menu_open =
            false;

        return true;
    }

    /*
     * Restart.
     */
    if (
        point_in_rect(
            desktop->mouse_x,
            desktop->mouse_y,
            x + 10,
            y + START_ACTION_Y,
            START_MENU_WIDTH - 20,
            START_ACTION_HEIGHT
        )
    ) {
        desktop->start_menu_open =
            false;

        desktop_restart();

        return true;
    }

    /*
     * Power off.
     */
    int power_y =
        START_ACTION_Y +
        START_ACTION_HEIGHT +
        START_ACTION_GAP;

    if (
        point_in_rect(
            desktop->mouse_x,
            desktop->mouse_y,
            x + 10,
            y + power_y,
            START_MENU_WIDTH - 20,
            START_ACTION_HEIGHT
        )
    ) {
        desktop->start_menu_open =
            false;

        desktop_power_off();

        return true;
    }

    /*
     * Clicking inside the menu consumes the click.
     */
    if (
        point_in_rect(
            desktop->mouse_x,
            desktop->mouse_y,
            x,
            y,
            START_MENU_WIDTH,
            START_MENU_HEIGHT
        )
    ) {
        return true;
    }

    /*
     * Clicking outside closes it.
     */
    desktop->start_menu_open =
        false;

    return false;
}

void desktop_mouse_button(
    desktop_t *desktop,
    bool left
)
{
    if (desktop == 0) {
        return;
    }

    if (
        left &&
        !desktop->previous_mouse_left
    ) {
        /*
         * Start Menu gets first chance.
         */
        if (
            desktop_start_menu_click(
                desktop
            )
        ) {
            desktop->previous_mouse_left =
                left;

            return;
        }

        /*
         * Taskbar.
         */
        if (
            desktop->mouse_y >=
            desktop->height -
            DESKTOP_TASKBAR_HEIGHT
        ) {
            desktop_taskbar_click(
                desktop
            );

            desktop->previous_mouse_left =
                left;

            return;
        }

        /*
         * Window controls/focus.
         */
        window_t *window =
            desktop_top_window(
                desktop,
                desktop->mouse_x,
                desktop->mouse_y
            );

        if (window != 0) {
            desktop_focus(
                desktop,
                window
            );

            /*
             * Close.
             */
            if (
                close_button_contains(
                    window,
                    desktop->mouse_x,
                    desktop->mouse_y
                )
            ) {
                window->visible =
                    false;

                window->minimized =
                    false;

                window->focused =
                    false;

                window->dragging =
                    false;

                if (
                    desktop->focused ==
                    window
                ) {
                    desktop->focused =
                        0;

                    desktop_focus_next(
                        desktop
                    );
                }

                desktop->dragging =
                    0;

                desktop->start_menu_open =
                    false;
            }

            /*
             * Minimize.
             */
            else if (
                minimize_button_contains(
                    window,
                    desktop->mouse_x,
                    desktop->mouse_y
                )
            ) {
                window->minimized =
                    true;

                window->focused =
                    false;

                if (
                    desktop->focused ==
                    window
                ) {
                    desktop->focused =
                        0;

                    desktop_focus_next(
                        desktop
                    );
                }

                desktop->dragging =
                    0;
            }

            /*
             * Title bar dragging.
             */
            else if (
                window_titlebar_contains(
                    window,
                    desktop->mouse_x,
                    desktop->mouse_y
                )
            ) {
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

            desktop->previous_mouse_left =
                left;

            return;
        }

        desktop->start_menu_open =
            false;
    }

    /*
     * Mouse release.
     */
    if (
        !left &&
        desktop->previous_mouse_left
    ) {
        if (
            desktop->dragging != 0
        ) {
            desktop->dragging->dragging =
                false;

            desktop_clamp_window(
                desktop,
                desktop->dragging
            );

            desktop->dragging =
                0;
        }
    }

    desktop->previous_mouse_left =
        left;
}
