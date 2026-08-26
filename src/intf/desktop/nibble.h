#ifndef FAZBEAROS_NIBBLE_H
#define FAZBEAROS_NIBBLE_H

#include <stdbool.h>

#include "desktop/window.h"

#define NIBBLE_BUFFER_SIZE 512

typedef struct {
    char text[NIBBLE_BUFFER_SIZE];

    int length;

    bool dirty;
    bool active;
} nibble_t;

void nibble_init(
    nibble_t *nibble
);

void nibble_update(
    nibble_t *nibble
);

void nibble_handle_key(
    nibble_t *nibble,
    char character
);

void nibble_render(
    const nibble_t *nibble,
    const window_t *window
);

#endif
