#include "desktop/window.h"

void window_init(
    window_t *window,
    const char *title,
    int x,
    int y,
    int width,
    int height
)
{
    if (window == 0) {
        return;
    }

    window->x =
        x;

    window->y =
        y;

    window->width =
        width;

    window->height =
        height;

    window->background =
        0xD8D8E0;

    window->border =
        0xFFFFFF;

    window->titlebar =
        0x303080;

    window->visible =
        true;

    window->focused =
        true;

    window->dragging =
        false;

    window->drag_offset_x =
        0;

    window->drag_offset_y =
        0;

    int i = 0;

    if (title != 0) {

        while (
            title[i] != '\0' &&
            i < WINDOW_TITLE_MAX - 1
        ) {

            window->title[i] =
                title[i];

            i++;
        }
    }

    window->title[i] =
        '\0';
}

bool window_contains(
    const window_t *window,
    int x,
    int y
)
{
    if (window == 0 ||
        !window->visible) {

        return false;
    }

    return
        x >= window->x &&
        y >= window->y &&
        x < window->x + window->width &&
        y < window->y + window->height;
}

bool window_titlebar_contains(
    const window_t *window,
    int x,
    int y
)
{
    if (window == 0 ||
        !window->visible) {

        return false;
    }

    return
        x >= window->x &&
        y >= window->y &&
        x < window->x + window->width &&
        y < window->y + 28;
}

void window_move(
    window_t *window,
    int x,
    int y
)
{
    if (window == 0) {
        return;
    }

    window->x =
        x;

    window->y =
        y;
}
