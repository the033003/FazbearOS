#ifndef FAZBEAROS_MOUSE_H
#define FAZBEAROS_MOUSE_H

#include <stdint.h>

#define MOUSE_BUTTON_LEFT    0x01
#define MOUSE_BUTTON_RIGHT   0x02
#define MOUSE_BUTTON_MIDDLE  0x04

struct mouse_state {
    int x;
    int y;

    int delta_x;
    int delta_y;

    uint8_t buttons;
    uint8_t previous_buttons;
};

void mouse_init(void);

void mouse_interrupt(void);

void mouse_poll(void);

void mouse_handle_byte(
    uint8_t value
);

void mouse_set_screen_size(
    int width,
    int height
);

const struct mouse_state*
mouse_get_state(void);

int mouse_event_available(void);

void mouse_clear_event(void);

#endif
