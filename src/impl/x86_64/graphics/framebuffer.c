#include "graphics.h"

#include <stddef.h>
#include <stdint.h>

static struct framebuffer framebuffer;

static int framebuffer_ready;

static uint8_t font[128][8] = {
    [0x20] = {
        0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00
    },

    [0x21] = {
        0x18, 0x18, 0x18, 0x18,
        0x18, 0x00, 0x18, 0x00
    },

    [0x2D] = {
        0x00, 0x00, 0x00, 0x7E,
        0x00, 0x00, 0x00, 0x00
    },

    [0x2E] = {
        0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x18, 0x00
    },

    [0x2F] = {
        0x06, 0x0C, 0x18, 0x30,
        0x60, 0xC0, 0x80, 0x00
    },

    [0x30] = {
        0x3C, 0x66, 0x6E, 0x76,
        0x66, 0x66, 0x3C, 0x00
    },

    [0x31] = {
        0x18, 0x38, 0x18, 0x18,
        0x18, 0x18, 0x7E, 0x00
    },

    [0x32] = {
        0x3C, 0x66, 0x06, 0x0C,
        0x18, 0x30, 0x7E, 0x00
    },

    [0x33] = {
        0x3C, 0x66, 0x06, 0x1C,
        0x06, 0x66, 0x3C, 0x00
    },

    [0x34] = {
        0x0C, 0x1C, 0x3C, 0x6C,
        0x7E, 0x0C, 0x0C, 0x00
    },

    [0x35] = {
        0x7E, 0x60, 0x7C, 0x06,
        0x06, 0x66, 0x3C, 0x00
    },

    [0x36] = {
        0x1C, 0x30, 0x60, 0x7C,
        0x66, 0x66, 0x3C, 0x00
    },

    [0x37] = {
        0x7E, 0x06, 0x0C, 0x18,
        0x30, 0x30, 0x30, 0x00
    },

    [0x38] = {
        0x3C, 0x66, 0x66, 0x3C,
        0x66, 0x66, 0x3C, 0x00
    },

    [0x39] = {
        0x3C, 0x66, 0x66, 0x3E,
        0x06, 0x0C, 0x38, 0x00
    },

    [0x3A] = {
        0x00, 0x18, 0x18, 0x00,
        0x00, 0x18, 0x18, 0x00
    },

    [0x3F] = {
        0x3C, 0x66, 0x06, 0x0C,
        0x18, 0x00, 0x18, 0x00
    },

    [0x41] = {
        0x18, 0x3C, 0x66, 0x66,
        0x7E, 0x66, 0x66, 0x00
    },

    [0x42] = {
        0x7C, 0x66, 0x66, 0x7C,
        0x66, 0x66, 0x7C, 0x00
    },

    [0x43] = {
        0x3C, 0x66, 0x60, 0x60,
        0x60, 0x66, 0x3C, 0x00
    },

    [0x44] = {
        0x78, 0x6C, 0x66, 0x66,
        0x66, 0x6C, 0x78, 0x00
    },

    [0x45] = {
        0x7E, 0x60, 0x60, 0x7C,
        0x60, 0x60, 0x7E, 0x00
    },

    [0x46] = {
        0x7E, 0x60, 0x60, 0x7C,
        0x60, 0x60, 0x60, 0x00
    },

    [0x47] = {
        0x3C, 0x66, 0x60, 0x6E,
        0x66, 0x66, 0x3C, 0x00
    },

    [0x48] = {
        0x66, 0x66, 0x66, 0x7E,
        0x66, 0x66, 0x66, 0x00
    },

    [0x49] = {
        0x3C, 0x18, 0x18, 0x18,
        0x18, 0x18, 0x3C, 0x00
    },

    [0x4A] = {
        0x1E, 0x0C, 0x0C, 0x0C,
        0x0C, 0x6C, 0x38, 0x00
    },

    [0x4B] = {
        0x66, 0x6C, 0x78, 0x70,
        0x78, 0x6C, 0x66, 0x00
    },

    [0x4C] = {
        0x60, 0x60, 0x60, 0x60,
        0x60, 0x60, 0x7E, 0x00
    },

    [0x4D] = {
        0x63, 0x77, 0x7F, 0x6B,
        0x63, 0x63, 0x63, 0x00
    },

    [0x4E] = {
        0x66, 0x76, 0x7E, 0x7E,
        0x6E, 0x66, 0x66, 0x00
    },

    [0x4F] = {
        0x3C, 0x66, 0x66, 0x66,
        0x66, 0x66, 0x3C, 0x00
    },

    [0x50] = {
        0x7C, 0x66, 0x66, 0x7C,
        0x60, 0x60, 0x60, 0x00
    },

    [0x51] = {
        0x3C, 0x66, 0x66, 0x66,
        0x6E, 0x3C, 0x0E, 0x00
    },

    [0x52] = {
        0x7C, 0x66, 0x66, 0x7C,
        0x78, 0x6C, 0x66, 0x00
    },

    [0x53] = {
        0x3C, 0x66, 0x60, 0x3C,
        0x06, 0x66, 0x3C, 0x00
    },

    [0x54] = {
        0x7E, 0x18, 0x18, 0x18,
        0x18, 0x18, 0x18, 0x00
    },

    [0x55] = {
        0x66, 0x66, 0x66, 0x66,
        0x66, 0x66, 0x3C, 0x00
    },

    [0x56] = {
        0x66, 0x66, 0x66, 0x66,
        0x66, 0x66, 0x3C, 0x18
    },

    [0x57] = {
        0x63, 0x63, 0x63, 0x6B,
        0x6B, 0x7F, 0x36, 0x00
    },

    [0x58] = {
        0x66, 0x66, 0x3C, 0x18,
        0x3C, 0x66, 0x66, 0x00
    },

    [0x59] = {
        0x66, 0x66, 0x66, 0x3C,
        0x18, 0x18, 0x18, 0x00
    },

    [0x5A] = {
        0x7E, 0x06, 0x0C, 0x18,
        0x30, 0x60, 0x7E, 0x00
    },

    [0x5F] = {
        0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0xFF, 0x00
    },

    [0x61] = {
        0x00, 0x00, 0x3C, 0x06,
        0x3E, 0x66, 0x3E, 0x00
    },

    [0x62] = {
        0x60, 0x60, 0x7C, 0x66,
        0x66, 0x66, 0x7C, 0x00
    },

    [0x63] = {
        0x00, 0x00, 0x3C, 0x66,
        0x60, 0x66, 0x3C, 0x00
    },

    [0x64] = {
        0x06, 0x06, 0x3E, 0x66,
        0x66, 0x66, 0x3E, 0x00
    },

    [0x65] = {
        0x00, 0x00, 0x3C, 0x66,
        0x7E, 0x60, 0x3C, 0x00
    },

    [0x66] = {
        0x1C, 0x36, 0x30, 0x7C,
        0x30, 0x30, 0x30, 0x00
    },

    [0x67] = {
        0x00, 0x00, 0x3E, 0x66,
        0x66, 0x3E, 0x06, 0x7C
    },

    [0x68] = {
        0x60, 0x60, 0x7C, 0x66,
        0x66, 0x66, 0x66, 0x00
    },

    [0x69] = {
        0x18, 0x00, 0x38, 0x18,
        0x18, 0x18, 0x3C, 0x00
    },

    [0x6A] = {
        0x06, 0x00, 0x0E, 0x06,
        0x06, 0x06, 0x66, 0x3C
    },

    [0x6B] = {
        0x60, 0x60, 0x66, 0x6C,
        0x78, 0x6C, 0x66, 0x00
    },

    [0x6C] = {
        0x38, 0x18, 0x18, 0x18,
        0x18, 0x18, 0x3C, 0x00
    },

    [0x6D] = {
        0x00, 0x00, 0x6C, 0x7E,
        0x6B, 0x6B, 0x63, 0x00
    },

    [0x6E] = {
        0x00, 0x00, 0x7C, 0x66,
        0x66, 0x66, 0x66, 0x00
    },

    [0x6F] = {
        0x00, 0x00, 0x3C, 0x66,
        0x66, 0x66, 0x3C, 0x00
    },

    [0x70] = {
        0x00, 0x00, 0x7C, 0x66,
        0x66, 0x7C, 0x60, 0x60
    },

    [0x71] = {
        0x00, 0x00, 0x3E, 0x66,
        0x66, 0x3E, 0x06, 0x06
    },

    [0x72] = {
        0x00, 0x00, 0x6C, 0x76,
        0x60, 0x60, 0x60, 0x00
    },

    [0x73] = {
        0x00, 0x00, 0x3E, 0x60,
        0x3C, 0x06, 0x7C, 0x00
    },

    [0x74] = {
        0x30, 0x30, 0x7C, 0x30,
        0x30, 0x36, 0x1C, 0x00
    },

    [0x75] = {
        0x00, 0x00, 0x66, 0x66,
        0x66, 0x66, 0x3E, 0x00
    },

    [0x76] = {
        0x00, 0x00, 0x66, 0x66,
        0x66, 0x3C, 0x18, 0x00
    },

    [0x77] = {
        0x00, 0x00, 0x63, 0x6B,
        0x6B, 0x7F, 0x36, 0x00
    },

    [0x78] = {
        0x00, 0x00, 0x66, 0x3C,
        0x18, 0x3C, 0x66, 0x00
    },

    [0x79] = {
        0x00, 0x00, 0x66, 0x66,
        0x66, 0x3E, 0x06, 0x7C
    },

    [0x7A] = {
        0x00, 0x00, 0x7E, 0x0C,
        0x18, 0x30, 0x7E, 0x00
    }
};

static void framebuffer_set_mode(
    uint32_t* address,
    uint32_t width,
    uint32_t height,
    uint32_t pitch,
    uint8_t red_position,
    uint8_t green_position,
    uint8_t blue_position,
    uint8_t reserved_position,
    uint8_t red_mask,
    uint8_t green_mask,
    uint8_t blue_mask,
    uint8_t reserved_mask
)
{
    framebuffer.address = address;
    framebuffer.width = width;
    framebuffer.height = height;
    framebuffer.pitch = pitch;

    framebuffer.red_position = red_position;
    framebuffer.green_position = green_position;
    framebuffer.blue_position = blue_position;
    framebuffer.reserved_position = reserved_position;

    framebuffer.red_mask = red_mask;
    framebuffer.green_mask = green_mask;
    framebuffer.blue_mask = blue_mask;
    framebuffer.reserved_mask = reserved_mask;

    framebuffer_ready = 1;
}

void graphics_init(
    uint64_t multiboot_information
)
{
    framebuffer_ready = 0;

    uint8_t* base =
        (uint8_t*)(uintptr_t)
        multiboot_information;

    uint32_t total_size =
        *(uint32_t*)base;

    uint8_t* current =
        base + 8;

    uint8_t* end =
        base + total_size;

    while (current < end) {
        uint32_t type =
            *(uint32_t*)current;

        uint32_t size =
            *(uint32_t*)(current + 4);

        if (type == 0) {
            break;
        }

        if (type == 8) {
            uint64_t address =
                *(uint64_t*)(current + 8);

            uint32_t pitch =
                *(uint32_t*)(current + 16);

            uint32_t width =
                *(uint32_t*)(current + 20);

            uint32_t height =
                *(uint32_t*)(current + 24);

            uint8_t depth =
                *(uint8_t*)(current + 28);

            uint8_t framebuffer_type =
                *(uint8_t*)(current + 29);

            if (framebuffer_type == 1 &&
                depth == 32 &&
                address != 0 &&
                width != 0 &&
                height != 0) {

                uint8_t* color_info =
                    current + 32;

                uint8_t red_position =
                    color_info[0];

                uint8_t red_mask =
                    color_info[1];

                uint8_t green_position =
                    color_info[2];

                uint8_t green_mask =
                    color_info[3];

                uint8_t blue_position =
                    color_info[4];

                uint8_t blue_mask =
                    color_info[5];

                framebuffer_set_mode(
                    (uint32_t*)(uintptr_t)address,
                    width,
                    height,
                    pitch,
                    red_position,
                    green_position,
                    blue_position,
                    24,
                    red_mask,
                    green_mask,
                    blue_mask,
                    8
                );

                break;
            }
        }

        if (size < 8) {
            break;
        }

        current +=
            (size + 7) & ~7u;
    }
}

const struct framebuffer* graphics_get_framebuffer(void)
{
    return &framebuffer;
}

int graphics_available(void)
{
    return framebuffer_ready;
}

void graphics_put_pixel(
    int32_t x,
    int32_t y,
    uint32_t color
)
{
    if (!framebuffer_ready) {
        return;
    }

    if (x < 0 ||
        y < 0 ||
        x >= (int32_t)framebuffer.width ||
        y >= (int32_t)framebuffer.height) {

        return;
    }

    uint8_t* address =
        (uint8_t*)framebuffer.address +
        (size_t)y * framebuffer.pitch +
        (size_t)x * 4;

    *(uint32_t*)address = color;
}

void graphics_fill_rect(
    int32_t x,
    int32_t y,
    int32_t width,
    int32_t height,
    uint32_t color
)
{
    if (width <= 0 ||
        height <= 0) {
        return;
    }

    for (int32_t current_y = y;
         current_y < y + height;
         current_y++) {

        for (int32_t current_x = x;
             current_x < x + width;
             current_x++) {

            graphics_put_pixel(
                current_x,
                current_y,
                color
            );
        }
    }
}

void graphics_rect(
    int32_t x,
    int32_t y,
    int32_t width,
    int32_t height,
    uint32_t color
)
{
    if (width <= 0 ||
        height <= 0) {
        return;
    }

    graphics_fill_rect(
        x,
        y,
        width,
        1,
        color
    );

    graphics_fill_rect(
        x,
        y + height - 1,
        width,
        1,
        color
    );

    graphics_fill_rect(
        x,
        y,
        1,
        height,
        color
    );

    graphics_fill_rect(
        x + width - 1,
        y,
        1,
        height,
        color
    );
}

void graphics_clear(
    uint32_t color
)
{
    if (!framebuffer_ready) {
        return;
    }

    graphics_fill_rect(
        0,
        0,
        (int32_t)framebuffer.width,
        (int32_t)framebuffer.height,
        color
    );
}

void graphics_draw_char(
    int32_t x,
    int32_t y,
    char character,
    uint32_t foreground,
    uint32_t background,
    uint8_t scale
)
{
    if (scale == 0) {
        scale = 1;
    }

    uint8_t code =
        (uint8_t)character;

    if (code >= 128) {
        code = '?';
    }

    for (int row = 0;
         row < 8;
         row++) {

        uint8_t bits =
            font[code][row];

        for (int column = 0;
             column < 8;
             column++) {

            uint32_t color =
                (bits &
                 (uint8_t)(1u << (7 - column)))
                    ? foreground
                    : background;

            graphics_fill_rect(
                x + column * scale,
                y + row * scale,
                scale,
                scale,
                color
            );
        }
    }
}

void graphics_draw_text(
    int32_t x,
    int32_t y,
    const char* text,
    uint32_t foreground,
    uint32_t background,
    uint8_t scale
)
{
    if (text == 0) {
        return;
    }

    int32_t start_x = x;

    if (scale == 0) {
        scale = 1;
    }

    while (*text != '\0') {
        if (*text == '\n') {
            x = start_x;
            y += 8 * scale;
            text++;
            continue;
        }

        graphics_draw_char(
            x,
            y,
            *text,
            foreground,
            background,
            scale
        );

        x += 8 * scale;
        text++;
    }
}

void graphics_present(void)
{
    /*
     * Rendering is currently direct-to-framebuffer.
     *
     * This function is intentionally retained as the
     * presentation boundary so a backbuffer can be added
     * later without changing the desktop API.
     */
}
