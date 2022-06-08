#include "print.h"

void kernel_main() {
    print_clear();
    print_set_color(PRINT_COLOR_WHITE, PRINT_COLOR_BLACK);
    print_str("Welcome to Fazbear OS\n\nCongrats on booting properly!");
}
