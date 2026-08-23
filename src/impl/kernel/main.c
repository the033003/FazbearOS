#include <stdint.h>

#include "device.h"
#include "fs.h"
#include "graphics.h"
#include "heap.h"
#include "interrupts.h"
#include "keyboard.h"
#include "log.h"
#include "memory.h"
#include "mouse.h"
#include "pic.h"
#include "print.h"
#include "timer.h"

#include "desktop/desktop.h"

static uint64_t boot_information;

static void kernel_banner(void)
{
    print_set_color(
        PRINT_COLOR_LIGHT_CYAN,
        PRINT_COLOR_BLACK
    );

    print_str(
        "\n"
        "========================================\n"
        "              FazbearOS                 \n"
        "========================================\n"
        "\n"
    );

    print_set_color(
        PRINT_COLOR_LIGHT_GRAY,
        PRINT_COLOR_BLACK
    );
}

static void initialize_kernel(void)
{
    log_init();

    log_info(
        "initializing memory subsystem"
    );

    memory_init(
        boot_information
    );

    log_info(
        "memory subsystem OK"
    );

    heap_init();

    log_info(
        "heap OK"
    );

    log_info(
        "initializing device subsystem"
    );

    device_subsystem_init();

    log_info(
        "device subsystem OK"
    );

    log_info(
        "initializing virtual filesystem"
    );

    vfs_init();

    struct fs_node* ram_root =
        ramfs_root();

    if (ram_root != 0) {

        vfs_mount_root(
            ram_root
        );

        log_info(
            "ramfs mounted"
        );
    }

    /*
     * Graphics must be initialized after the heap because
     * the software back buffer is allocated from the heap.
     */
    log_info(
        "initializing graphics"
    );

    graphics_init(
        boot_information
    );

    if (!graphics_available()) {

        log_info(
            "framebuffer UNAVAILABLE"
        );

        print_str(
            "\n"
            "FRAMEBUFFER UNAVAILABLE\n"
        );

        for (;;) {
            __asm__ volatile (
                "hlt"
            );
        }
    }

    const struct framebuffer*
        framebuffer =
        graphics_get_framebuffer();

    if (framebuffer == 0 ||
        framebuffer->address == 0 ||
        framebuffer->width == 0 ||
        framebuffer->height == 0) {

        log_info(
            "framebuffer INVALID"
        );

        print_str(
            "\n"
            "FRAMEBUFFER INVALID\n"
        );

        for (;;) {
            __asm__ volatile (
                "hlt"
            );
        }
    }

    log_info(
        "framebuffer AVAILABLE"
    );

    /*
     * Initialize the PIC first.
     *
     * Interrupts are still disabled at this point.
     */
    log_info(
        "initializing PIC"
    );

    pic_init();

    /*
     * Install the IDT.
     *
     * interrupts_init() no longer executes STI.
     */
    log_info(
        "initializing IDT"
    );

    interrupts_init();

    log_info(
        "IDT loaded"
    );

    /*
     * Initialize all IRQ-driven devices before enabling
     * interrupts. This prevents the mouse from interrupting
     * the CPU while mouse_init() is synchronously talking
     * to the PS/2 controller.
     */
    log_info(
        "initializing timer"
    );

    timer_init(
        100
    );

    log_info(
        "timer initialized"
    );

    log_info(
        "initializing keyboard"
    );

    keyboard_init();

    log_info(
        "keyboard initialized"
    );

    log_info(
        "initializing mouse"
    );

    mouse_init();

    log_info(
        "mouse initialized"
    );

    /*
     * Everything that can generate a hardware interrupt is
     * now initialized.
     *
     * It is finally safe to enable interrupts.
     */
    __asm__ volatile (
        "sti"
    );

    log_info(
        "interrupts enabled"
    );
}

void kernel_main(
    uint64_t multiboot_information
)
{
    boot_information =
        multiboot_information;

    print_clear();

    kernel_banner();

    print_str(
        "kernel_main entered\n"
        "\n"
    );

    initialize_kernel();

    print_str(
        "\n"
        "========================================\n"
        "             FazbearOS Desktop          \n"
        "========================================\n"
        "\n"
    );

    log_info(
        "starting desktop"
    );

    desktop_t desktop;

    const struct framebuffer*
        framebuffer =
        graphics_get_framebuffer();

    desktop_init(
        &desktop,
        (int)framebuffer->width,
        (int)framebuffer->height
    );

    log_info(
        "desktop initialized"
    );

    /*
     * Main desktop loop.
     *
     * Input is collected from interrupt handlers.
     * desktop_update() consumes the latest mouse event.
     * Rendering always goes to the software back buffer.
     * graphics_present() copies that buffer to the real
     * framebuffer.
     */
    for (;;) {

        desktop_update(
            &desktop
        );

        desktop_render(
            &desktop
        );

        graphics_present();

        /*
         * Interrupts are enabled, so HLT sleeps until the
         * next timer, keyboard, or mouse interrupt.
         */
        __asm__ volatile (
            "hlt"
        );
    }
}
