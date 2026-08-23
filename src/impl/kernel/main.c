#include <stdint.h>

#include "device.h"
#include "fs.h"
#include "graphics.h"
#include "heap.h"
#include "interrupts.h"
#include "keyboard.h"
#include "log.h"
#include "memory.h"
#include "pic.h"
#include "print.h"
#include "timer.h"

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

    } else {

        log_info(
            "ramfs unavailable"
        );
    }

    /*
     * Initialize graphics AFTER the heap,
     * because the software framebuffer is
     * allocated from the kernel heap.
     */
    log_info(
        "initializing graphics"
    );

    graphics_init(
        boot_information
    );

    log_info(
        "graphics_init returned"
    );

    if (!graphics_available()) {

        log_info(
            "framebuffer UNAVAILABLE"
        );

        print_str(
            "\n"
            "FRAMEBUFFER UNAVAILABLE\n"
        );

        while (1) {
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

        while (1) {
            __asm__ volatile (
                "hlt"
            );
        }
    }

    log_info(
        "framebuffer AVAILABLE"
    );

    /*
     * Do NOT initialize interrupts, timer,
     * keyboard, mouse, or desktop yet.
     *
     * We are deliberately isolating the
     * framebuffer from everything else.
     */
}

static void framebuffer_test(void)
{
    log_info(
        "starting framebuffer test"
    );

    /*
     * Entire frame goes into the software
     * buffer.
     */
    graphics_clear(
        0x202040
    );

    /*
     * Large cyan rectangle.
     */
    graphics_fill_rect(
        50,
        50,
        400,
        200,
        0x00AAFF
    );

    /*
     * White border.
     */
    graphics_rect(
        50,
        50,
        400,
        200,
        0xFFFFFF
    );

    /*
     * Text.
     */
    graphics_draw_text(
        80,
        100,
        "FAZBEAROS",
        0xFFFFFF,
        0x00AAFF,
        2
    );

    graphics_draw_text(
        80,
        135,
        "GRAPHICS OK",
        0xFFFFFF,
        0x00AAFF,
        2
    );

    /*
     * Present exactly once.
     */
    graphics_present();

    log_info(
        "framebuffer test presented"
    );

    /*
     * Stay here.
     *
     * No interrupts.
     * No mouse.
     * No desktop.
     * No repeated framebuffer copies.
     */
    while (1) {
        __asm__ volatile (
            "hlt"
        );
    }
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
        "          FRAMEBUFFER TEST              \n"
        "========================================\n"
        "\n"
    );

    framebuffer_test();
}
