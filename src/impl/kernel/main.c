#include <stdint.h>

#include "device.h"
#include "fs.h"
#include "heap.h"
#include "interrupts.h"
#include "keyboard.h"
#include "log.h"
#include "memory.h"
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

    memory_init(boot_information);

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
        vfs_mount_root(ram_root);
    }

    log_info(
        "initializing interrupt subsystem"
    );

    interrupts_init();

    pic_init();

    log_info(
        "initializing timer"
    );

    timer_init(100);

    log_info(
        "initializing keyboard"
    );

    keyboard_init();
}

static void start_desktop(void)
{
    desktop_t desktop;

    log_info(
        "initializing desktop"
    );

    /*
     * The desktop currently uses the framebuffer's
     * configured resolution.
     *
     * The desktop implementation owns rendering after
     * initialization and will eventually become the
     * compositor/window manager.
     */
    desktop_init(
        &desktop,
        1024,
        768
    );

    log_info(
        "desktop initialized"
    );

    /*
     * The kernel remains alive while the desktop runs.
     *
     * Future iterations will replace this polling loop
     * with timer-driven scheduling and application
     * processes.
     */
    while (1) {
        desktop_update(
            &desktop
        );

        desktop_render(
            &desktop
        );

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
     * The old shell is intentionally no longer started
     * directly from the kernel.
     *
     * It will become a graphical terminal application.
     */
    start_desktop();
}
