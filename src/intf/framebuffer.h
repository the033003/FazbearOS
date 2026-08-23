cat > src/intf/framebuffer.h <<'EOF'
#ifndef FAZBEAROS_FRAMEBUFFER_H
#define FAZBEAROS_FRAMEBUFFER_H

#include <stdint.h>

struct framebuffer {
    uint32_t* address;
    uint32_t width;
    uint32_t height;
    uint32_t pitch;
    uint8_t bpp;
};

void framebuffer_init(void);

struct framebuffer* framebuffer_get(void);

void framebuffer_clear(uint32_t color);

void framebuffer_put_pixel(
    uint32_t x,
    uint32_t y,
    uint32_t color
);

void framebuffer_fill_rect(
    uint32_t x,
    uint32_t y,
    uint32_t width,
    uint32_t height,
    uint32_t color
);

void framebuffer_draw_rect(
    uint32_t x,
    uint32_t y,
    uint32_t width,
    uint32_t height,
    uint32_t color
);

#endif
EOF
