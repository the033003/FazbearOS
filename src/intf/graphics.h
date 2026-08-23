#pragma once

#include <stddef.h>
#include <stdint.h>

struct framebuffer {
    uint32_t* address;

    uint32_t width;
    uint32_t height;
    uint32_t pitch;

    uint8_t red_position;
    uint8_t green_position;
    uint8_t blue_position;
    uint8_t reserved_position;

    uint8_t red_mask;
    uint8_t green_mask;
    uint8_t blue_mask;
    uint8_t reserved_mask;
};

void graphics_init(
    uint64_t multiboot_information
);

const struct framebuffer* graphics_get_framebuffer(void);

int graphics_available(void);

void graphics_clear(
    uint32_t color
);

void graphics_put_pixel(
    int32_t x,
    int32_t y,
    uint32_t color
);

void graphics_fill_rect(
    int32_t x,
    int32_t y,
    int32_t width,
    int32_t height,
    uint32_t color
);

void graphics_rect(
    int32_t x,
    int32_t y,
    int32_t width,
    int32_t height,
    uint32_t color
);

void graphics_draw_char(
    int32_t x,
    int32_t y,
    char character,
    uint32_t foreground,
    uint32_t background,
    uint8_t scale
);

void graphics_draw_text(
    int32_t x,
    int32_t y,
    const char* text,
    uint32_t foreground,
    uint32_t background,
    uint8_t scale
);

void graphics_present(void);
