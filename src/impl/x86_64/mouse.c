#include "mouse.h"
#include "io.h"
#include "pic.h"

#define PS2_STATUS 0x64
#define PS2_CMD    0x64
#define PS2_DATA   0x60

/*
 * Mouse sensitivity.
 *
 * A value of 2 makes the cursor travel roughly twice as far
 * for the same PS/2 movement packet.
 *
 * Keep this fairly small. Large values make the cursor feel
 * twitchy, especially when the mouse reports larger deltas.
 */
#define MOUSE_SENSITIVITY 2

static struct mouse_state state;

static uint8_t packet[3];
static uint8_t packet_idx;

static int event_available;

static int screen_w = 1024;
static int screen_h = 768;

static void wait_write(void)
{
    for (int i = 0; i < 100000; i++) {
        if ((inb(PS2_STATUS) & 2) == 0) {
            return;
        }
    }
}

static int wait_read(void)
{
    for (int i = 0; i < 100000; i++) {
        if (inb(PS2_STATUS) & 1) {
            return 1;
        }
    }

    return 0;
}

static void mouse_write(uint8_t value)
{
    wait_write();

    /*
     * 0xD4 tells the PS/2 controller that the following
     * byte is intended for the auxiliary mouse device.
     */
    outb(PS2_CMD, 0xD4);

    wait_write();

    outb(PS2_DATA, value);
}

static int mouse_read(uint8_t *value)
{
    if (!wait_read()) {
        return 0;
    }

    *value = inb(PS2_DATA);

    return 1;
}

static int mouse_cmd(uint8_t command)
{
    uint8_t response;

    mouse_write(command);

    if (!mouse_read(&response)) {
        return 0;
    }

    return response == 0xFA;
}

static int clamp_x(int x)
{
    if (x < 0) {
        return 0;
    }

    if (x >= screen_w) {
        return screen_w - 1;
    }

    return x;
}

static int clamp_y(int y)
{
    if (y < 0) {
        return 0;
    }

    if (y >= screen_h) {
        return screen_h - 1;
    }

    return y;
}

void mouse_init(void)
{
    state.x = screen_w / 2;
    state.y = screen_h / 2;

    state.delta_x = 0;
    state.delta_y = 0;

    state.buttons = 0;
    state.previous_buttons = 0;

    packet_idx = 0;
    event_available = 0;

    /*
     * IRQ12 is initially masked while the PS/2 device
     * is being configured.
     */
    pic_mask(12);

    /*
     * Disable the mouse while configuring it.
     */
    mouse_cmd(0xF5);

    /*
     * Enable the auxiliary PS/2 port.
     */
    outb(PS2_CMD, 0xA8);

    /*
     * Read the controller configuration byte.
     */
    outb(PS2_CMD, 0x20);

    uint8_t config = 0;

    if (wait_read()) {
        config = inb(PS2_DATA);
    }

    /*
     * Enable IRQ12.
     *
     * Clear the mouse clock-disable bit.
     */
    config |= 0x02;
    config &= (uint8_t)~0x20;

    /*
     * Write the updated controller configuration.
     */
    outb(PS2_CMD, 0x60);

    wait_write();

    outb(PS2_DATA, config);

    /*
     * Reset the mouse.
     */
    mouse_write(0xFF);

    uint8_t response;

    if (
        mouse_read(&response) &&
        response == 0xFA
    ) {
        /*
         * Reset completion response.
         */
        if (
            mouse_read(&response) &&
            response == 0xAA
        ) {
            /*
             * Mouse ID.
             *
             * For the standard three-byte mouse this
             * should normally be 0x00.
             */
            mouse_read(&response);
        }
    }

    /*
     * Restore standard mouse settings.
     */
    mouse_cmd(0xF6);

    /*
     * Enable data reporting.
     */
    mouse_cmd(0xF4);

    /*
     * Now allow IRQ12 to reach the kernel.
     */
    pic_unmask(12);
}

void mouse_handle_byte(uint8_t value)
{
    /*
     * A valid first byte must have bit 3 set.
     *
     * This lets us recover synchronization if the packet
     * stream starts in the middle of a packet.
     */
    if (
        packet_idx == 0 &&
        (value & 0x08) == 0
    ) {
        return;
    }

    packet[packet_idx++] = value;

    if (packet_idx < 3) {
        return;
    }

    packet_idx = 0;

    uint8_t flags = packet[0];

    /*
     * Bits 6 and 7 indicate X/Y overflow.
     * Discard that packet rather than applying corrupted
     * movement.
     */
    if (flags & 0xC0) {
        return;
    }

    /*
     * PS/2 movement values are signed 9-bit values.
     */
    int16_t dx = packet[1];
    int16_t dy = packet[2];

    if (flags & 0x10) {
        dx |= (int16_t)0xFF00;
    }

    if (flags & 0x20) {
        dy |= (int16_t)0xFF00;
    }

    /*
     * Preserve button transitions immediately.
     */
    state.previous_buttons = state.buttons;
    state.buttons = flags & 0x07;

    /*
     * Apply modest sensitivity.
     *
     * The raw PS/2 deltas are intentionally retained as
     * integers so there is no floating-point work in the
     * kernel mouse path.
     */
    int32_t move_x =
        (int32_t)dx *
        MOUSE_SENSITIVITY;

    int32_t move_y =
        (int32_t)dy *
        MOUSE_SENSITIVITY;

    /*
     * Accumulate movement instead of replacing it.
     *
     * This is important: multiple mouse packets can arrive
     * between desktop_update() calls. Previously, only the
     * final packet's delta survived.
     */
    state.delta_x += move_x;
    state.delta_y -= move_y;

    /*
     * Update the absolute cursor position using the same
     * accumulated movement.
     */
    state.x = clamp_x(
        state.x + move_x
    );

    state.y = clamp_y(
        state.y - move_y
    );

    event_available = 1;
}

void mouse_interrupt(void)
{
    uint8_t status = inb(PS2_STATUS);

    /*
     * No data available.
     */
    if ((status & 1) == 0) {
        return;
    }

    /*
     * Data is not from the auxiliary mouse device.
     */
    if ((status & 0x20) == 0) {
        return;
    }

    mouse_handle_byte(
        inb(PS2_DATA)
    );
}

void mouse_poll(void)
{
    /*
     * Drain all currently available PS/2 data.
     *
     * This prevents the desktop update loop from seeing
     * only one packet when the mouse generated several
     * packets since the previous frame.
     */
    while (inb(PS2_STATUS) & 1) {
        uint8_t status = inb(PS2_STATUS);

        if ((status & 0x20) == 0) {
            /*
             * The controller has keyboard data rather than
             * mouse data. Leave it for the keyboard driver.
             */
            break;
        }

        mouse_handle_byte(
            inb(PS2_DATA)
        );
    }
}

void mouse_set_screen_size(
    int width,
    int height
)
{
    if (width < 1) {
        width = 1;
    }

    if (height < 1) {
        height = 1;
    }

    screen_w = width;
    screen_h = height;

    state.x = clamp_x(state.x);
    state.y = clamp_y(state.y);
}

const struct mouse_state *mouse_get_state(void)
{
    return &state;
}

int mouse_event_available(void)
{
    return event_available;
}

void mouse_clear_event(void)
{
    /*
     * The accumulated movement has now been consumed
     * by the desktop.
     */
    state.delta_x = 0;
    state.delta_y = 0;

    event_available = 0;

    /*
     * Make the next packet's button state compare against
     * the current state rather than an old packet.
     */
    state.previous_buttons = state.buttons;
}
