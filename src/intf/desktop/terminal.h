#ifndef FAZBEAROS_TERMINAL_H
#define FAZBEAROS_TERMINAL_H

#include <stdbool.h>

#include "desktop/window.h"

#define TERMINAL_OUTPUT_SIZE 8192
#define TERMINAL_INPUT_SIZE  128
#define TERMINAL_HISTORY_SIZE 16

typedef struct {
    char output[TERMINAL_OUTPUT_SIZE];
    int output_length;

    char input[TERMINAL_INPUT_SIZE];
    int input_length;

    char history[TERMINAL_HISTORY_SIZE][TERMINAL_INPUT_SIZE];
    int history_count;
    int history_position;

    bool active;
    bool dirty;
} terminal_t;

void terminal_init(
    terminal_t *terminal
);

void terminal_update(
    terminal_t *terminal
);

void terminal_handle_key(
    terminal_t *terminal,
    char character
);

void terminal_render(
    const terminal_t *terminal,
    const window_t *window
);

#endif
