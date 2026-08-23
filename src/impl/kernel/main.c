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
    __asm__ volatile (
        "cli"
    );

    log_init();

    log_info("initializing memory subsystem");
    memory_init(boot_information);
    log_info("memory subsystem OK");

    heap_init();
    log_info("heap OK");

    log_info("initializing device subsystem");
    device_subsystem_init();
    log_info("device subsystem OK");

    log_info("initializing virtual filesystem");
    vfs_init();

    struct fs_node* ram_root = ramfs_root();
    if (ram_root != 0) {
        vfs_mount_root(ram_root);
        log_info("ramfs mounted");
    }

    log_info("initializing graphics");
    graphics_init(boot_information);

    if (!graphics_available()) {
        log_info("framebuffer UNAVAILABLE");
        print_str("\nFRAMEBUFFER UNAVAILABLE\n");
        for (;;) {
            __asm__ volatile ("cli\nhlt");
        }
    }

    const struct framebuffer* framebuffer = graphics_get_framebuffer();
    if (framebuffer == 0 ||
        framebuffer->address == 0 ||
        framebuffer->width == 0 ||
        framebuffer->height == 0) {

        log_info("framebuffer INVALID");
        print_str("\nFRAMEBUFFER INVALID\n");
        for (;;) {
            __asm__ volatile ("cli\nhlt");
        }
    }

    log_info("framebuffer AVAILABLE");

    log_info("initializing PIC");
    pic_init();
    log_info("PIC initialized");

    log_info("initializing IDT");
    interrupts_init();
    log_info("IDT initialized");

    log_info("initializing timer");
    timer_init(100);
    log_info("timer initialized");

    log_info("initializing keyboard");
    keyboard_init();
    log_info("keyboard initialized");

    log_info("initializing mouse");
    mouse_init();
    log_info("mouse initialized");

    __asm__ volatile ("sti");
    log_info("interrupts enabled");
}

void kernel_main(uint64_t multiboot_information)
{
    boot_information = multiboot_information;

    print_clear();
    kernel_banner();

    print_str("kernel_main entered\n\n");

    initialize_kernel();

    print_str(
        "\n"
        "========================================\n"
        "             FazbearOS Desktop          \n"
        "========================================\n"
        "\n"
    );

    log_info("starting desktop");

    const struct framebuffer* framebuffer = graphics_get_framebuffer();
    if (framebuffer == 0) {
        for (;;) {
            __asm__ volatile ("cli\nhlt");
        }
    }

    desktop_t desktop;
    desktop_init(
        &desktop,
        (int)framebuffer->width,
        (int)framebuffer->height
    );

    log_info("desktop initialized");

    for (;;) {
        mouse_poll();
        desktop_update(&desktop);
        desktop_render(&desktop);
        graphics_present();
    }
}