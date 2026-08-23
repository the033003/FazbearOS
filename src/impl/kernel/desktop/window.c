#include "desktop/window.h"

static void string_copy(
    char *destination,
    const char *source,
    int max_length
) {
    int i;

    if (!destination || !source || max_length <= 0) {
        return;
    }

    for (i = 0; i < max_length - 1 && source[i]; ++i) {
        destination[i] = source[i];
    }

    destination[i] = '\0';
}

void window_init(
    window_t *window,
    const char *title,
    int x,
    int y,
    int width,
    int height
) {
    if (!window) {
        return;
    }

    window->x = x;
    window->y = y;
    window->width = width;
    window->height = height;

    window->background = 0x00121820;
    window->border = 0x00304050;
    window->titlebar = 0x001b2633;

    window->visible = true;
    window->focused = false;
    window->dragging = false;

    window->drag_offset_x = 0;
    window->drag_offset_y = 0;

    string_copy(window->title, title, WINDOW_TITLE_MAX);
}

bool window_contains(
    const window_t *window,
    int x,
    int y
) {
    if (!window || !window->visible) {
        return false;
    }

    return x >= window->x &&
           x < window->x + window->width &&
           y >= window->y &&
           y < window->y + window->height;
}

bool window_titlebar_contains(
    const window_t *window,
    int x,
    int y
) {
    if (!window_contains(window, x, y)) {
        return false;
    }

    return y >= window->y &&
           y < window->y + 24;
}

void window_move(
    window_t *window,
    int x,
    int y
) {
    if (!window) {
        return;
    }

    window->x = x;
    window->y = y;
}
