#include "print.h"
#include "shell.h"

void kernel_main(void)
{
    print_clear();

    print_set_color(
        PRINT_COLOR_WHITE,
        PRINT_COLOR_BLACK
    );

    shell_start();

    while (1) {
        __asm__ volatile ("hlt");
    }
}
