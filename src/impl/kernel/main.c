#include <stdint.h>

#include "desktop/desktop.h"

#include "graphics.h"
#include "memory.h"
#include "heap.h"

#include "pic.h"
#include "interrupts.h"
#include "keyboard.h"
#include "mouse.h"
#include "timer.h"

static void halt_cpu(void)
{
    __asm__ volatile ("hlt");
}

void kernel_main(
    uint64_t multiboot_information
)
{
    /*
     * Core memory.
     */
    memory_init(
        multiboot_information
    );

    heap_init();

    /*
     * Graphics must come after the heap because
     * framebuffer.c allocates its software backbuffer.
     */
    graphics_init(
        multiboot_information
    );

    /*
     * Hardware input.
     */
    pic_init();
    interrupts_init();

    keyboard_init();
    mouse_init();

    /*
     * A small heartbeat for future desktop timing,
     * animations and application scheduling.
     */
    timer_init(100);

    /*
     * Enable hardware interrupts only after
     * every interrupt source has been configured.
     */
    __asm__ volatile ("sti");

    desktop_t desktop;

    if (graphics_available()) {
        const struct framebuffer *framebuffer =
            graphics_get_framebuffer();

        if (
            framebuffer != 0 &&
            framebuffer->width > 0 &&
            framebuffer->height > 0
        ) {
            desktop_init(
                &desktop,
                (int)framebuffer->width,
                (int)framebuffer->height
            );

            for (;;) {
                desktop_update(
                    &desktop
                );

                desktop_render(
                    &desktop
                );

                halt_cpu();
            }
        }
    }

    /*
     * Graphics failed. Keep the machine alive rather
     * than executing into an invalid state.
     */
    for (;;) {
        halt_cpu();
    }
}
