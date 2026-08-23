#ifndef FAZBEAROS_WINDOW_H
#define FAZBEAROS_WINDOW_H

#include <stdint.h>
#include <stdbool.h>

#define WINDOW_TITLE_MAX 64
#define WINDOW_TITLEBAR_HEIGHT 28

typedef struct {
    int x;
    int y;

    int width;
    int height;

    uint32_t background;
    uint32_t border;
    uint32_t titlebar;

    bool visible;
    bool focused;
    bool dragging;

    int drag_offset_x;
    int drag_offset_y;

    char title[WINDOW_TITLE_MAX];
} window_t;

void window_init(
    window_t *window,
    const char *title,
    int x,
    int y,
    int width,
    int height
);

bool window_contains(
    const window_t *window,
    int x,
    int y
);

bool window_titlebar_contains(
    const window_t *window,
    int x,
    int y
);

void window_move(
    window_t *window,
    int x,
    int y
);

#endif
