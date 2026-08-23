#include "mouse.h"

#include "io.h"

#define PS2_STATUS  0x64
#define PS2_COMMAND 0x64
#define PS2_DATA    0x60

#define PS2_STATUS_OUTPUT_FULL 0x01
#define PS2_STATUS_INPUT_FULL  0x02
#define PS2_STATUS_AUX_DATA    0x20

#define MOUSE_PACKET_SIZE 3

#define MOUSE_ACK          0xFA
#define MOUSE_SELF_TEST_OK 0xAA

static struct mouse_state state;

static uint8_t packet[MOUSE_PACKET_SIZE];
static uint8_t packet_index;
static int event_available;

static void wait_write(void)
{
    for (uint32_t i = 0; i < 100000; i++) {
        if ((inb(PS2_STATUS) & PS2_STATUS_INPUT_FULL) == 0) {
            return;
        }
    }
}

static int wait_read(void)
{
    for (uint32_t i = 0; i < 100000; i++) {
        if ((inb(PS2_STATUS) & PS2_STATUS_OUTPUT_FULL) != 0) {
            return 1;
        }
    }

    return 0;
}

static void controller_command(uint8_t command)
{
    wait_write();
    outb(PS2_COMMAND, command);
}

static void controller_write(uint8_t value)
{
    wait_write();
    outb(PS2_DATA, value);
}

static void mouse_write(uint8_t value)
{
    /*
     * Tell the PS/2 controller that the next byte written to
     * port 0x60 belongs to the auxiliary device.
     */
    controller_command(0xD4);
    controller_write(value);
}

static int mouse_read(uint8_t *value)
{
    if (!wait_read()) {
        return 0;
    }

    *value = inb(PS2_DATA);
    return 1;
}

static int mouse_command(uint8_t command)
{
    uint8_t response;

    mouse_write(command);

    if (!mouse_read(&response)) {
        return 0;
    }

    return response == MOUSE_ACK;
}

static void flush_output(void)
{
    for (uint32_t i = 0; i < 1000; i++) {
        uint8_t status = inb(PS2_STATUS);

        if ((status & PS2_STATUS_OUTPUT_FULL) == 0) {
            return;
        }

        (void)inb(PS2_DATA);
    }
}

static void process_packet(void)
{
    uint8_t flags = packet[0];

    /*
     * Bit 3 is always set in a valid standard PS/2 packet.
     */
    if ((flags & 0x08) == 0) {
        return;
    }

    /*
     * Ignore X/Y overflow packets.
     */
    if ((flags & 0xC0) != 0) {
        return;
    }

    int16_t dx = (int16_t)packet[1];
    int16_t dy = (int16_t)packet[2];

    if (flags & 0x10) {
        dx |= (int16_t)0xFF00;
    }

    if (flags & 0x20) {
        dy |= (int16_t)0xFF00;
    }

    state.previous_buttons = state.buttons;
    state.buttons = flags & 0x07;

    state.delta_x = dx;
    state.delta_y = -dy;

    state.x += dx;
    state.y -= dy;

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
     * Disable mouse reporting while configuring it.
     */
    (void)mouse_command(0xF5);

    /*
     * Enable the auxiliary PS/2 port.
     */
    controller_command(0xA8);

    /*
     * Read controller configuration byte.
     */
    controller_command(0x20);

    uint8_t configuration = 0;

    if (wait_read()) {
        configuration = inb(PS2_DATA);
    }

    /*
     * Enable IRQ12.
     */
    configuration |= 0x02;

    /*
     * Enable auxiliary clock.
     */
    configuration &= (uint8_t)~0x20;

    /*
     * Write controller configuration byte.
     */
    controller_command(0x60);
    controller_write(configuration);

    /*
     * Remove any stale bytes before talking to the mouse.
     */
    flush_output();

    /*
     * Reset mouse.
     *
     * Expected:
     *
     *   FA
     *   AA
     *   00
     */
    mouse_write(0xFF);

    uint8_t response;

    if (mouse_read(&response)) {
        if (response == MOUSE_ACK) {
            if (mouse_read(&response)) {
                if (response == MOUSE_SELF_TEST_OK) {
                    /*
                     * Mouse ID.
                     */
                    (void)mouse_read(&response);
                }
            }
        }
    }

    /*
     * Set default parameters.
     */
    (void)mouse_command(0xF6);

    /*
     * Enable mouse reporting.
     */
    (void)mouse_command(0xF4);

    packet_index = 0;
    event_available = 0;
}

void mouse_interrupt(void)
{
    uint8_t status = inb(PS2_STATUS);

    /*
     * Nothing waiting.
     */
    if ((status & PS2_STATUS_OUTPUT_FULL) == 0) {
        return;
    }

    /*
     * The byte must come from the auxiliary device.
     *
     * If this is keyboard data, leave it for the keyboard ISR.
     */
    if ((status & PS2_STATUS_AUX_DATA) == 0) {
        return;
    }

    uint8_t value = inb(PS2_DATA);

    mouse_handle_byte(value);
}

void mouse_handle_byte(uint8_t value)
{
    /*
     * Synchronize on the first packet byte.
     */
    if (packet_index == 0) {
        if ((value & 0x08) == 0) {
            return;
        }
    }

    packet[packet_index] = value;
    packet_index++;

    if (packet_index == MOUSE_PACKET_SIZE) {
        packet_index = 0;
        process_packet();
    }
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
    event_available = 0;
}
