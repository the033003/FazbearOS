#include "graphics.h"

#include <stddef.h>
#include <stdint.h>

/*
 * Multiboot2 tag header.
 */
struct multiboot_tag {
    uint32_t type;
    uint32_t size;
};

/*
 * Multiboot2 framebuffer tag.
 *
 * Layout:
 *
 *   u32 type
 *   u32 size
 *   u64 framebuffer_addr
 *   u32 framebuffer_pitch
 *   u32 framebuffer_width
 *   u32 framebuffer_height
 *   u8  framebuffer_bpp
 *   u8  framebuffer_type
 *   u16 reserved
 *   color information...
 */
struct multiboot_tag_framebuffer {
    uint32_t type;
    uint32_t size;

    uint64_t framebuffer_addr;
    uint32_t framebuffer_pitch;
    uint32_t framebuffer_width;
    uint32_t framebuffer_height;

    uint8_t framebuffer_bpp;
    uint8_t framebuffer_type;

    uint16_t reserved;

    union {
        struct {
            uint32_t framebuffer_palette_num_colors;
            uint8_t framebuffer_palette[0];
        };

        struct {
            uint8_t framebuffer_red_field_position;
            uint8_t framebuffer_red_mask_size;

            uint8_t framebuffer_green_field_position;
            uint8_t framebuffer_green_mask_size;

            uint8_t framebuffer_blue_field_position;
            uint8_t framebuffer_blue_mask_size;

            uint8_t framebuffer_reserved_field_position;
            uint8_t framebuffer_reserved_mask_size;
        };
    };
};

#define MULTIBOOT_TAG_END         0
#define MULTIBOOT_TAG_FRAMEBUFFER 8

#define MULTIBOOT_FRAMEBUFFER_INDEXED 0
#define MULTIBOOT_FRAMEBUFFER_RGB     1
#define MULTIBOOT_FRAMEBUFFER_TEXT    2

static struct framebuffer framebuffer;

static uint32_t* back_buffer;
static uint64_t back_buffer_size;

static int available;

/*
 * Convert a 0xRRGGBB color into the actual framebuffer pixel layout.
 */
static uint32_t make_color(
    uint32_t color
)
{
    uint32_t red =
        (color >> 16) & 0xFFu;

    uint32_t green =
        (color >> 8) & 0xFFu;

    uint32_t blue =
        color & 0xFFu;

    uint32_t result =
        0;

    if (framebuffer.red_mask != 0) {
        uint32_t value =
            red;

        if (framebuffer.red_mask < 8) {
            value >>=
                8 - framebuffer.red_mask;
        }

        result |=
            (value &
             ((1u << framebuffer.red_mask) - 1u))
            << framebuffer.red_position;
    }

    if (framebuffer.green_mask != 0) {
        uint32_t value =
            green;

        if (framebuffer.green_mask < 8) {
            value >>=
                8 - framebuffer.green_mask;
        }

        result |=
            (value &
             ((1u << framebuffer.green_mask) - 1u))
            << framebuffer.green_position;
    }

    if (framebuffer.blue_mask != 0) {
        uint32_t value =
            blue;

        if (framebuffer.blue_mask < 8) {
            value >>=
                8 - framebuffer.blue_mask;
        }

        result |=
            (value &
             ((1u << framebuffer.blue_mask) - 1u))
            << framebuffer.blue_position;
    }

    return result;
}

/*
 * Copy the software back buffer into the real framebuffer.
 *
 * The framebuffer pitch is measured in bytes and may be larger
 * than width * 4.
 */
static void copy_framebuffer(void)
{
    if (!available ||
        back_buffer == 0 ||
        framebuffer.address == 0) {
        return;
    }

    if (framebuffer.width == 0 ||
        framebuffer.height == 0) {
        return;
    }

    if (framebuffer.pitch <
        framebuffer.width * 4u) {
        return;
    }

    uint32_t width =
        framebuffer.width;

    uint32_t height =
        framebuffer.height;

    uint32_t pitch_pixels =
        framebuffer.pitch / 4u;

    for (uint32_t y = 0;
         y < height;
         y++) {

        uint32_t* destination =
            framebuffer.address +
            ((uint64_t)y *
             pitch_pixels);

        uint32_t* source =
            back_buffer +
            ((uint64_t)y *
             width);

        for (uint32_t x = 0;
             x < width;
             x++) {

            destination[x] =
                source[x];
        }
    }
}

void graphics_init(
    uint64_t multiboot_information
)
{
    available = 0;

    framebuffer.address = 0;

    framebuffer.width = 0;
    framebuffer.height = 0;
    framebuffer.pitch = 0;

    framebuffer.red_position = 0;
    framebuffer.green_position = 0;
    framebuffer.blue_position = 0;
    framebuffer.reserved_position = 0;

    framebuffer.red_mask = 0;
    framebuffer.green_mask = 0;
    framebuffer.blue_mask = 0;
    framebuffer.reserved_mask = 0;

    back_buffer = 0;
    back_buffer_size = 0;

    if (multiboot_information == 0) {
        return;
    }

    /*
     * A Multiboot2 information structure begins with:
     *
     *   uint32_t total_size;
     *   uint32_t reserved;
     *
     * The FIRST TAG starts at offset 8.
     *
     * This is critical. The previous implementation incorrectly
     * started parsing at offset 0, causing total_size to be
     * interpreted as a tag type and the framebuffer tag to never
     * be reached.
     */
    uint8_t* mbi =
        (uint8_t*)(uintptr_t)
        multiboot_information;

    uint32_t total_size =
        *(uint32_t*)mbi;

    if (total_size < 16) {
        return;
    }

    /*
     * Multiboot2 structures must be 8-byte aligned.
     *
     * The total size includes the initial 8-byte header.
     */
    if ((total_size & 7u) != 0) {
        return;
    }

    uint8_t* address =
        mbi + 8;

    uint8_t* end =
        mbi + total_size;

    while (address + 8 <= end) {

        struct multiboot_tag* tag =
            (struct multiboot_tag*)address;

        if (tag->size < 8) {
            return;
        }

        if (address + tag->size > end) {
            return;
        }

        if (tag->type ==
            MULTIBOOT_TAG_END) {
            break;
        }

        if (tag->type ==
            MULTIBOOT_TAG_FRAMEBUFFER) {

            /*
             * The common framebuffer portion ends
             * after the reserved u16, at 32 bytes.
             */
            if (tag->size < 32) {
                return;
            }

            struct multiboot_tag_framebuffer* fb =
                (struct multiboot_tag_framebuffer*)
                tag;

            /*
             * We require a direct RGB framebuffer.
             */
            if (fb->framebuffer_type !=
                MULTIBOOT_FRAMEBUFFER_RGB) {
                return;
            }

            /*
             * The renderer works with 32-bit pixels.
             */
            if (fb->framebuffer_bpp != 32) {
                return;
            }

            if (fb->framebuffer_addr == 0) {
                return;
            }

            if (fb->framebuffer_width == 0 ||
                fb->framebuffer_height == 0) {
                return;
            }

            /*
             * Validate the framebuffer pitch.
             */
            uint64_t minimum_pitch =
                (uint64_t)
                fb->framebuffer_width *
                4u;

            if ((uint64_t)
                fb->framebuffer_pitch <
                minimum_pitch) {
                return;
            }

            /*
             * Validate the complete framebuffer size
             * before storing any derived values.
             */
            uint64_t framebuffer_size =
                (uint64_t)
                fb->framebuffer_pitch *
                (uint64_t)
                fb->framebuffer_height;

            if (framebuffer_size == 0) {
                return;
            }

            /*
             * Make sure the framebuffer address plus its
             * size does not wrap around.
             */
            if (fb->framebuffer_addr >
                UINT64_MAX - framebuffer_size) {
                return;
            }

            framebuffer.address =
                (uint32_t*)(uintptr_t)
                fb->framebuffer_addr;

            framebuffer.width =
                fb->framebuffer_width;

            framebuffer.height =
                fb->framebuffer_height;

            framebuffer.pitch =
                fb->framebuffer_pitch;

            framebuffer.red_position =
                fb->framebuffer_red_field_position;

            framebuffer.green_position =
                fb->framebuffer_green_field_position;

            framebuffer.blue_position =
                fb->framebuffer_blue_field_position;

            framebuffer.reserved_position =
                fb->framebuffer_reserved_field_position;

            framebuffer.red_mask =
                fb->framebuffer_red_mask_size;

            framebuffer.green_mask =
                fb->framebuffer_green_mask_size;

            framebuffer.blue_mask =
                fb->framebuffer_blue_mask_size;

            framebuffer.reserved_mask =
                fb->framebuffer_reserved_mask_size;

            /*
             * Calculate the software back-buffer size.
             */
            uint64_t pixels =
                (uint64_t)
                framebuffer.width *
                (uint64_t)
                framebuffer.height;

            if (pixels >
                ((uint64_t)SIZE_MAX /
                 sizeof(uint32_t))) {

                framebuffer.address = 0;
                return;
            }

            back_buffer_size =
                pixels *
                sizeof(uint32_t);

            /*
             * The software back buffer is allocated after
             * the physical framebuffer has been validated.
             */
            extern void* kmalloc(
                size_t size
            );

            back_buffer =
                (uint32_t*)
                kmalloc(
                    (size_t)
                    back_buffer_size
                );

            if (back_buffer == 0) {
                framebuffer.address = 0;
                back_buffer_size = 0;

                return;
            }

            /*
             * Start with a completely black back buffer.
             */
            for (uint64_t i = 0;
                 i < pixels;
                 i++) {

                back_buffer[i] =
                    0;
            }

            available =
                1;

            return;
        }

        /*
         * Advance from the CURRENT tag.
         *
         * Multiboot2 tags are individually padded to
         * an 8-byte boundary.
         */
        uint32_t next =
            (tag->size + 7u) &
            ~7u;

        if (next < 8) {
            return;
        }

        if (address + next > end) {
            return;
        }

        address += next;
    }
}

const struct framebuffer*
graphics_get_framebuffer(void)
{
    return &framebuffer;
}

int graphics_available(void)
{
    return available;
}

void graphics_clear(
    uint32_t color
)
{
    if (!available ||
        back_buffer == 0) {
        return;
    }

    uint32_t pixel =
        make_color(color);

    uint64_t pixels =
        (uint64_t)
        framebuffer.width *
        (uint64_t)
        framebuffer.height;

    for (uint64_t i = 0;
         i < pixels;
         i++) {

        back_buffer[i] =
            pixel;
    }
}

void graphics_put_pixel(
    int32_t x,
    int32_t y,
    uint32_t color
)
{
    if (!available ||
        back_buffer == 0) {
        return;
    }

    if (x < 0 ||
        y < 0 ||
        x >= (int32_t)framebuffer.width ||
        y >= (int32_t)framebuffer.height) {
        return;
    }

    back_buffer[
        ((uint64_t)y *
         framebuffer.width) +
        (uint32_t)x
    ] =
        make_color(color);
}

void graphics_fill_rect(
    int32_t x,
    int32_t y,
    int32_t width,
    int32_t height,
    uint32_t color
)
{
    if (!available ||
        back_buffer == 0) {
        return;
    }

    if (width <= 0 ||
        height <= 0) {
        return;
    }

    int32_t x_start =
        x;

    int32_t y_start =
        y;

    int32_t x_end =
        x + width;

    int32_t y_end =
        y + height;

    if (x_start < 0) {
        x_start = 0;
    }

    if (y_start < 0) {
        y_start = 0;
    }

    if (x_end >
        (int32_t)framebuffer.width) {

        x_end =
            (int32_t)framebuffer.width;
    }

    if (y_end >
        (int32_t)framebuffer.height) {

        y_end =
            (int32_t)framebuffer.height;
    }

    if (x_start >= x_end ||
        y_start >= y_end) {
        return;
    }

    uint32_t pixel =
        make_color(color);

    for (int32_t py = y_start;
         py < y_end;
         py++) {

        uint32_t* row =
            back_buffer +
            ((uint64_t)py *
             framebuffer.width);

        for (int32_t px = x_start;
             px < x_end;
             px++) {

            row[px] =
                pixel;
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

    /*
     * 5x5 font.
     *
     * Entries cover ASCII 32 through 127.
     */
    static const uint8_t font[96][5] = {

        [1] = {0,0,0,0,0},

        ['A' - 32] = {14,17,17,31,17},
        ['B' - 32] = {30,17,30,17,30},
        ['C' - 32] = {15,16,16,16,15},
        ['D' - 32] = {30,17,17,17,30},
        ['E' - 32] = {31,16,30,16,31},
        ['F' - 32] = {31,16,30,16,16},
        ['G' - 32] = {15,16,23,17,15},
        ['H' - 32] = {17,17,31,17,17},
        ['I' - 32] = {31,4,4,4,31},
        ['J' - 32] = {7,2,2,18,12},
        ['K' - 32] = {17,18,28,18,17},
        ['L' - 32] = {16,16,16,16,31},
        ['M' - 32] = {17,27,21,17,17},
        ['N' - 32] = {17,25,21,19,17},
        ['O' - 32] = {14,17,17,17,14},
        ['P' - 32] = {30,17,30,16,16},
        ['Q' - 32] = {14,17,17,21,10},
        ['R' - 32] = {30,17,30,18,17},
        ['S' - 32] = {15,16,14,1,30},
        ['T' - 32] = {31,4,4,4,4},
        ['U' - 32] = {17,17,17,17,14},
        ['V' - 32] = {17,17,17,10,4},
        ['W' - 32] = {17,17,21,27,17},
        ['X' - 32] = {17,10,4,10,17},
        ['Y' - 32] = {17,10,4,4,4},
        ['Z' - 32] = {31,2,4,8,31},

        ['0' - 32] = {14,17,19,21,14},
        ['1' - 32] = {4,12,4,4,14},
        ['2' - 32] = {14,17,2,4,31},
        ['3' - 32] = {30,1,6,1,30},
        ['4' - 32] = {2,6,10,31,2},
        ['5' - 32] = {31,16,30,1,30},
        ['6' - 32] = {14,16,30,17,14},
        ['7' - 32] = {31,1,2,4,4},
        ['8' - 32] = {14,17,14,17,14},
        ['9' - 32] = {14,17,15,1,14},

        ['.' - 32] = {0,0,0,0,4},
        ['-' - 32] = {0,0,31,0,0},
        ['_' - 32] = {0,0,0,0,31},
        [':' - 32] = {0,4,0,4,0},
        ['/' - 32] = {1,2,4,8,16}
    };

    uint32_t fg =
        make_color(foreground);

    uint32_t bg =
        make_color(background);

    /*
     * Fill the character background first.
     */
    graphics_fill_rect(
        x,
        y,
        5 * scale,
        5 * scale,
        background
    );

    uint8_t character_code =
        (uint8_t)character;

    if (character_code < 32 ||
        character_code > 127) {
        return;
    }

    const uint8_t* glyph =
        font[
            character_code - 32
        ];

    for (int row = 0;
         row < 5;
         row++) {

        uint8_t bits =
            glyph[row];

        for (int col = 0;
             col < 5;
             col++) {

            uint32_t pixel =
                (bits &
                 (1u << (4 - col)))
                ? fg
                : bg;

            graphics_fill_rect(
                x + col * scale,
                y + row * scale,
                scale,
                scale,
                pixel
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

    int32_t cursor_x =
        x;

    while (*text != '\0') {

        graphics_draw_char(
            cursor_x,
            y,
            *text,
            foreground,
            background,
            scale
        );

        cursor_x +=
            6 * scale;

        text++;
    }
}

void graphics_present(void)
{
    if (!available) {
        return;
    }

    copy_framebuffer();
}
