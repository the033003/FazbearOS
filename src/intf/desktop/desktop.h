#ifndef FAZBEAROS_DESKTOP_H
#define FAZBEAROS_DESKTOP_H

#include <stdint.h>
#include <stdbool.h>

#include "desktop/window.h"
#include "desktop/nibble.h"
#include "desktop/terminal.h"

#define DESKTOP_MAX_WINDOWS 16

typedef struct {
    int width;
    int height;

    int mouse_x;
    int mouse_y;

    bool mouse_left;
    bool previous_mouse_left;

    bool start_menu_open;

    window_t *windows[
        DESKTOP_MAX_WINDOWS
    ];

    int window_count;

    window_t *focused;
    window_t *dragging;

    nibble_t nibble;
    window_t nibble_window;

    terminal_t terminal;
    window_t terminal_window;
} desktop_t;

void desktop_init(
    desktop_t *desktop,
    int width,
    int height
);

void desktop_update(
    desktop_t *desktop
);

void desktop_render(
    desktop_t *desktop
);

void desktop_mouse_move(
    desktop_t *desktop,
    int dx,
    int dy
);

void desktop_mouse_button(
    desktop_t *desktop,
    bool left
);

#endif
