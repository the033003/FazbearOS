#include "mouse.h"
#include "io.h"
#include "pic.h"

#define PS2_STATUS 0x64
#define PS2_CMD    0x64
#define PS2_DATA   0x60

static struct mouse_state state;
static uint8_t packet[3];
static uint8_t packet_idx;
static int event_available;
static int screen_w = 1024, screen_h = 768;

static void wait_write(void) {
    for (int i = 0; i < 100000; i++)
        if ((inb(PS2_STATUS) & 2) == 0) return;
}

static int wait_read(void) {
    for (int i = 0; i < 100000; i++)
        if (inb(PS2_STATUS) & 1) return 1;
    return 0;
}

static void mouse_write(uint8_t v) {
    wait_write();
    outb(PS2_CMD, 0xD4);
    wait_write();
    outb(PS2_DATA, v);
}

static int mouse_read(uint8_t *v) {
    if (!wait_read()) return 0;
    *v = inb(PS2_DATA);
    return 1;
}

static int mouse_cmd(uint8_t cmd) {
    uint8_t r;
    mouse_write(cmd);
    if (!mouse_read(&r)) return 0;
    return r == 0xFA;
}

void mouse_init(void) {
    state.x = screen_w / 2;
    state.y = screen_h / 2;
    state.delta_x = state.delta_y = 0;
    state.buttons = state.previous_buttons = 0;
    packet_idx = 0;
    event_available = 0;

    pic_mask(12);

    mouse_cmd(0xF5);                /* disable */
    outb(PS2_CMD, 0xA8);            /* enable aux */

    outb(PS2_CMD, 0x20);
    uint8_t cfg = 0;
    if (wait_read()) cfg = inb(PS2_DATA);
    cfg |= 0x02;
    cfg &= ~0x20;
    outb(PS2_CMD, 0x60);
    wait_write();
    outb(PS2_DATA, cfg);

    mouse_write(0xFF);              /* reset */
    uint8_t r;
    if (mouse_read(&r) && r == 0xFA)
        if (mouse_read(&r) && r == 0xAA)
            mouse_read(&r);

    mouse_cmd(0xF6);                /* defaults */
    mouse_cmd(0xF4);                /* enable */

    pic_unmask(12);
}

void mouse_handle_byte(uint8_t v) {
    if (packet_idx == 0 && (v & 0x08) == 0) return;
    packet[packet_idx++] = v;
    if (packet_idx == 3) {
        packet_idx = 0;
        uint8_t flags = packet[0];
        if (flags & 0xC0) return;

        int16_t dx = packet[1];
        int16_t dy = packet[2];
        if (flags & 0x10) dx |= 0xFF00;
        if (flags & 0x20) dy |= 0xFF00;

        state.previous_buttons = state.buttons;
        state.buttons = flags & 0x07;
        state.delta_x = dx;
        state.delta_y = -dy;
        state.x += dx;
        state.y -= dy;

        if (state.x < 0) state.x = 0;
        if (state.y < 0) state.y = 0;
        if (state.x >= screen_w) state.x = screen_w - 1;
        if (state.y >= screen_h) state.y = screen_h - 1;

        event_available = 1;
    }
}

void mouse_interrupt(void) {
    uint8_t status = inb(PS2_STATUS);
    if ((status & 1) == 0) return;
    if ((status & 0x20) == 0) return;
    mouse_handle_byte(inb(PS2_DATA));
}

void mouse_poll(void) {
    while (inb(PS2_STATUS) & 1) {
        if (inb(PS2_STATUS) & 0x20)
            mouse_handle_byte(inb(PS2_DATA));
        else
            break;
    }
}

void mouse_set_screen_size(int w, int h) {
    if (w < 1) w = 1;
    if (h < 1) h = 1;
    screen_w = w;
    screen_h = h;
}

const struct mouse_state *mouse_get_state(void) { return &state; }
int mouse_event_available(void) { return event_available; }
void mouse_clear_event(void) { event_available = 0; }