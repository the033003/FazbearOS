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
#include "shell.h"
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

    heap_init();

    log_info(
        "initializing device subsystem"
    );

    device_subsystem_init();

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
    }

    log_info(
        "initializing graphics"
    );

    graphics_init(
        boot_information
    );

    if (graphics_available()) {
        const struct framebuffer* framebuffer =
            graphics_get_framebuffer();

        log_info(
            "graphics framebuffer initialized"
        );

        print_str(
            "Framebuffer: "
        );

        print_str(
            "available\n"
        );

        (void)framebuffer;
    } else {
        log_info(
            "graphics framebuffer unavailable"
        );
    }

    log_info(
        "initializing interrupt subsystem"
    );

    interrupts_init();

    pic_init();

    log_info(
        "initializing timer"
    );

    timer_init(
        100
    );

    log_info(
        "initializing keyboard"
    );

    keyboard_init();

    log_info(
        "initializing mouse"
    );

    mouse_init();
}

static void start_desktop(void)
{
    desktop_t desktop;

    const struct framebuffer* framebuffer =
        graphics_get_framebuffer();

    int width = 1024;
    int height = 768;

    if (graphics_available() &&
        framebuffer != 0 &&
        framebuffer->width != 0 &&
        framebuffer->height != 0) {

        width =
            (int)framebuffer->width;

        height =
            (int)framebuffer->height;
    }

    log_info(
        "initializing desktop"
    );

    desktop_init(
        &desktop,
        width,
        height
    );

    log_info(
        "desktop initialized"
    );

    while (1) {
        desktop_update(
            &desktop
        );

        desktop_render(
            &desktop
        );

        graphics_present();

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

    initialize_kernel();

    print_str(
        "\n"
    );

    log_info(
        "kernel initialization complete"
    );

    print_str(
        "\n"
    );

    /*
     * The shell will become a graphical
     * terminal application.
     */
    start_desktop();
}
