#include "mouse.h"

#include "io.h"

#define PS2_STATUS  0x64
#define PS2_COMMAND 0x64
#define PS2_DATA    0x60

#define MOUSE_PACKET_SIZE 3

static struct mouse_state state;

static uint8_t packet[
    MOUSE_PACKET_SIZE
];

static uint8_t packet_index;

static int event_available;

static void wait_write(void)
{
    for (uint32_t i = 0;
         i < 100000;
         i++) {

        if ((inb(PS2_STATUS) & 0x02) == 0) {
            return;
        }
    }
}

static int wait_read(void)
{
    for (uint32_t i = 0;
         i < 100000;
         i++) {

        if ((inb(PS2_STATUS) & 0x01) != 0) {
            return 1;
        }
    }

    return 0;
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

static void process_packet(void)
{
    uint8_t flags =
        packet[0];

    /*
     * Bit 3 must always be set.
     */
    if ((flags & 0x08) == 0) {
        return;
    }

    /*
     * Ignore overflow packets.
     */
    if ((flags & 0xC0) != 0) {
        return;
    }

    int16_t dx =
        (int16_t)packet[1];

    int16_t dy =
        (int16_t)packet[2];

    /*
     * X sign bit.
     */
    if (flags & 0x10) {
        dx |= (int16_t)0xFF00;
    }

    /*
     * Y sign bit.
     */
    if (flags & 0x20) {
        dy |= (int16_t)0xFF00;
    }

    state.previous_buttons =
        state.buttons;

    state.buttons =
        flags & 0x07;

    state.delta_x =
        dx;

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

    event_available =
        1;
}

void mouse_init(void)
{
    state.x =
        512;

    state.y =
        384;

    state.delta_x =
        0;

    state.delta_y =
        0;

    state.buttons =
        0;

    state.previous_buttons =
        0;

    packet_index =
        0;

    event_available =
        0;

    /*
     * Enable PS/2 auxiliary device.
     */
    wait_write();

    outb(
        PS2_COMMAND,
        0xA8
    );

    /*
     * Read controller configuration.
     */
    wait_write();

    outb(
        PS2_COMMAND,
        0x20
    );

    uint8_t configuration =
        0;

    if (wait_read()) {
        configuration =
            inb(PS2_DATA);
    }

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

    /*
     * Write configuration.
     */
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

    /*
     * Consume reset ACK.
     */
    if (wait_read()) {
        (void)inb(PS2_DATA);
    }

    /*
     * Consume self-test result.
     */
    if (wait_read()) {
        (void)inb(PS2_DATA);
    }

    /*
     * Set defaults.
     */
    mouse_write(
        0xF6
    );

    if (wait_read()) {
        (void)inb(PS2_DATA);
    }

    /*
     * Enable reporting.
     */
    mouse_write(
        0xF4
    );

    if (wait_read()) {
        (void)inb(PS2_DATA);
    }
}

void mouse_interrupt(void)
{
    uint8_t status =
        inb(PS2_STATUS);

    /*
     * No data.
     */
    if ((status & 0x01) == 0) {
        return;
    }

    /*
     * Data belongs to the auxiliary device.
     */
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
     * Synchronize on packet byte 1.
     */
    if (packet_index == 0) {

        if ((value & 0x08) == 0) {
            return;
        }
    }

    packet[
        packet_index++
    ] = value;

    if (packet_index ==
        MOUSE_PACKET_SIZE) {

        packet_index =
            0;

        process_packet();
    }
}

const struct mouse_state*
mouse_get_state(void)
{
    return &state;
}

int mouse_event_available(void)
{
    return event_available;
}

void mouse_clear_event(void)
{
    event_available =
        0;
}
