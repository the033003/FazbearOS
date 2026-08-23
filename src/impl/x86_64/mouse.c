#include "mouse.h"

#include "io.h"

#define PS2_STATUS 0x64
#define PS2_COMMAND 0x64
#define PS2_DATA 0x60

#define MOUSE_PACKET_SIZE 3

static struct mouse_state state;

static uint8_t packet[3];

static uint8_t packet_index;

static int event_available;

static void wait_write(void)
{
    for (uint32_t i = 0;
         i < 100000;
         i++) {

        if ((inb(PS2_STATUS) & 2) == 0) {
            return;
        }
    }
}

static void wait_read(void)
{
    for (uint32_t i = 0;
         i < 100000;
         i++) {

        if ((inb(PS2_STATUS) & 1) != 0) {
            return;
        }
    }
}

static void mouse_write(
    uint8_t value
)
{
    wait_write();

    outb(
        PS2_COMMAND,
        0xD4
    );

    wait_write();

    outb(
        PS2_DATA,
        value
    );
}

static uint8_t mouse_read(void)
{
    wait_read();

    return inb(
        PS2_DATA
    );
}

static void process_packet(void)
{
    uint8_t flags =
        packet[0];

    /*
     * Bit 3 is always set in a valid
     * standard PS/2 mouse packet.
     */
    if ((flags & 0x08) == 0) {
        packet_index = 0;
        return;
    }

    /*
     * Ignore packets with X/Y overflow.
     */
    if ((flags & 0xC0) != 0) {
        packet_index = 0;
        return;
    }

    int16_t dx =
        (int16_t)packet[1];

    int16_t dy =
        (int16_t)packet[2];

    /*
     * Sign extend X.
     */
    if (flags & 0x10) {
        dx |=
            (int16_t)0xFF00;
    }

    /*
     * Sign extend Y.
     */
    if (flags & 0x20) {
        dy |=
            (int16_t)0xFF00;
    }

    state.previous_buttons =
        state.buttons;

    state.buttons =
        flags & 0x07;

    state.delta_x =
        dx;

    /*
     * PS/2 Y movement is positive upward.
     * Desktop coordinates are positive downward.
     */
    state.delta_y =
        -dy;

    state.x +=
        dx;

    state.y +=
        -dy;

    if (state.x < 0) {
        state.x = 0;
    }

    if (state.y < 0) {
        state.y = 0;
    }

    event_available = 1;
}

void mouse_init(void)
{
    state.x = 512;
    state.y = 384;

    state.delta_x = 0;
    state.delta_y = 0;

    state.buttons = 0;
    state.previous_buttons = 0;

    packet_index = 0;
    event_available = 0;

    /*
     * Enable the auxiliary PS/2 device.
     */
    wait_write();

    outb(
        PS2_COMMAND,
        0xA8
    );

    /*
     * Read controller configuration byte.
     */
    wait_write();

    outb(
        PS2_COMMAND,
        0x20
    );

    uint8_t configuration =
        mouse_read();

    /*
     * Enable IRQ12.
     */
    configuration |=
        0x02;

    /*
     * Enable mouse clock.
     */
    configuration &=
        (uint8_t)~0x20;

    wait_write();

    outb(
        PS2_COMMAND,
        0x60
    );

    wait_write();

    outb(
        PS2_DATA,
        configuration
    );

    /*
     * Reset mouse.
     */
    mouse_write(
        0xFF
    );

    mouse_read();
    mouse_read();

    /*
     * Set defaults.
     */
    mouse_write(
        0xF6
    );

    mouse_read();

    /*
     * Enable data reporting.
     */
    mouse_write(
        0xF4
    );

    mouse_read();
}

void mouse_interrupt(void)
{
    /*
     * Only read the data port when the
     * controller reports mouse data.
     *
     * Bit 5 of status indicates that the
     * byte came from the auxiliary device.
     */
    uint8_t status =
        inb(PS2_STATUS);

    if ((status & 0x01) == 0) {
        return;
    }

    if ((status & 0x20) == 0) {
        return;
    }

    mouse_handle_byte(
        inb(PS2_DATA)
    );
}

void mouse_handle_byte(
    uint8_t value
)
{
    /*
     * Synchronize to the first byte of a
     * standard three-byte packet.
     */
    if (packet_index == 0 &&
        (value & 0x08) == 0) {

        return;
    }

    packet[
        packet_index++
    ] = value;

    if (packet_index ==
        MOUSE_PACKET_SIZE) {

        packet_index = 0;

        process_packet();
    }
}

const struct mouse_state* mouse_get_state(void)
{
    return &state;
}

int mouse_event_available(void)
{
    return event_available;
}

void mouse_clear_event(void)
{
    event_available = 0;
}
