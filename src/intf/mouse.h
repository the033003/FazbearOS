#ifndef FAZBEAROS_MOUSE_H
#define FAZBEAROS_MOUSE_H

#include <stdint.h>

enum mouse_button {
    MOUSE_BUTTON_LEFT   = 1,
    MOUSE_BUTTON_RIGHT  = 2,
    MOUSE_BUTTON_MIDDLE = 4
};

struct mouse_state {
    int32_t x;
    int32_t y;

    int32_t delta_x;
    int32_t delta_y;

    uint8_t buttons;
    uint8_t previous_buttons;
};

void mouse_init(void);

void mouse_interrupt(void);

void mouse_handle_byte(uint8_t value);

const struct mouse_state *mouse_get_state(void);

int mouse_event_available(void);

void mouse_clear_event(void);

#endif
