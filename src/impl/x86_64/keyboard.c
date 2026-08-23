#include "keyboard.h"
#include "io.h"

#define KEYBOARD_BUFFER_SIZE 256

static volatile char buffer[KEYBOARD_BUFFER_SIZE];

static volatile uint16_t buffer_read = 0;
static volatile uint16_t buffer_write = 0;

static volatile int shift_pressed = 0;
static volatile int extended_scancode = 0;

static const char normal_map[128] = {
    [0x02] = '1',
    [0x03] = '2',
    [0x04] = '3',
    [0x05] = '4',
    [0x06] = '5',
    [0x07] = '6',
    [0x08] = '7',
    [0x09] = '8',
    [0x0A] = '9',
    [0x0B] = '0',
    [0x0C] = '-',
    [0x0D] = '=',
    [0x0E] = '\b',
    [0x0F] = '\t',

    [0x10] = 'q',
    [0x11] = 'w',
    [0x12] = 'e',
    [0x13] = 'r',
    [0x14] = 't',
    [0x15] = 'y',
    [0x16] = 'u',
    [0x17] = 'i',
    [0x18] = 'o',
    [0x19] = 'p',
    [0x1A] = '[',
    [0x1B] = ']',
    [0x1C] = '\n',

    [0x1E] = 'a',
    [0x1F] = 's',
    [0x20] = 'd',
    [0x21] = 'f',
    [0x22] = 'g',
    [0x23] = 'h',
    [0x24] = 'j',
    [0x25] = 'k',
    [0x26] = 'l',
    [0x27] = ';',
    [0x28] = '\'',
    [0x29] = '`',

    [0x2B] = '\\',

    [0x2C] = 'z',
    [0x2D] = 'x',
    [0x2E] = 'c',
    [0x2F] = 'v',
    [0x30] = 'b',
    [0x31] = 'n',
    [0x32] = 'm',
    [0x33] = ',',
    [0x34] = '.',
    [0x35] = '/',

    [0x39] = ' '
};

static const char shifted_map[128] = {
    [0x02] = '!',
    [0x03] = '@',
    [0x04] = '#',
    [0x05] = '$',
    [0x06] = '%',
    [0x07] = '^',
    [0x08] = '&',
    [0x09] = '*',
    [0x0A] = '(',
    [0x0B] = ')',
    [0x0C] = '_',
    [0x0D] = '+',

    [0x10] = 'Q',
    [0x11] = 'W',
    [0x12] = 'E',
    [0x13] = 'R',
    [0x14] = 'T',
    [0x15] = 'Y',
    [0x16] = 'U',
    [0x17] = 'I',
    [0x18] = 'O',
    [0x19] = 'P',
    [0x1A] = '{',
    [0x1B] = '}',

    [0x1E] = 'A',
    [0x1F] = 'S',
    [0x20] = 'D',
    [0x21] = 'F',
    [0x22] = 'G',
    [0x23] = 'H',
    [0x24] = 'J',
    [0x25] = 'K',
    [0x26] = 'L',
    [0x27] = ':',
    [0x28] = '"',
    [0x29] = '~',

    [0x2B] = '|',

    [0x2C] = 'Z',
    [0x2D] = 'X',
    [0x2E] = 'C',
    [0x2F] = 'V',
    [0x30] = 'B',
    [0x31] = 'N',
    [0x32] = 'M',
    [0x33] = '<',
    [0x34] = '>',
    [0x35] = '?',

    [0x39] = ' '
};

static void buffer_push(char character)
{
    uint16_t next =
        (uint16_t)(
            (buffer_write + 1) %
            KEYBOARD_BUFFER_SIZE
        );

    if (next == buffer_read) {
        return;
    }

    buffer[buffer_write] = character;

    buffer_write = next;
}

void keyboard_init(void)
{
    buffer_read = 0;
    buffer_write = 0;

    shift_pressed = 0;
    extended_scancode = 0;

    /*
     * Enable keyboard IRQ generation.
     */
    uint8_t command = inb(0x64);

    if ((command & 1) == 0) {
        /*
         * The keyboard controller command byte
         * is configured below only when needed.
         */
    }

    outb(0x64, 0xAE);
}

void keyboard_interrupt(void)
{
    uint8_t scancode = inb(0x60);

    if (scancode == 0xE0) {
        extended_scancode = 1;
        return;
    }

    if (scancode == 0x2A ||
        scancode == 0x36) {

        shift_pressed = 1;
        return;
    }

    if (scancode == 0xAA ||
        scancode == 0xB6) {

        shift_pressed = 0;
        return;
    }

    /*
     * Ignore key-release events.
     */
    if (scancode & 0x80) {
        extended_scancode = 0;
        return;
    }

    if (extended_scancode) {
        /*
         * Arrow keys and other extended keys will be
         * exposed as escape sequences later.
         */
        if (scancode == 0x48) {
            buffer_push('\x01');
        } else if (scancode == 0x50) {
            buffer_push('\x02');
        } else if (scancode == 0x4B) {
            buffer_push('\x03');
        } else if (scancode == 0x4D) {
            buffer_push('\x04');
        }

        extended_scancode = 0;

        return;
    }

    if (scancode >= 128) {
        return;
    }

    char character;

    if (shift_pressed) {
        character = shifted_map[scancode];
    } else {
        character = normal_map[scancode];
    }

    if (character != 0) {
        buffer_push(character);
    }
}

int keyboard_available(void)
{
    return buffer_read != buffer_write;
}

char keyboard_get_char(void)
{
    while (!keyboard_available()) {
        __asm__ volatile ("hlt");
    }

    char character = buffer[buffer_read];

    buffer_read =
        (uint16_t)(
            (buffer_read + 1) %
            KEYBOARD_BUFFER_SIZE
        );

    return character;
}

void keyboard_reboot(void)
{
    uint8_t status;

    do {
        status = inb(0x64);
    } while (status & 0x02);

    outb(0x64, 0xFE);

    for (;;) {
        __asm__ volatile ("cli; hlt");
    }
}
