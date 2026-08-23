#include "shell.h"

#include "keyboard.h"
#include "print.h"
#include "rtc.h"
#include "timer.h"

#define COMMAND_MAX_LENGTH 128

static int string_equals(
    const char* a,
    const char* b
)
{
    size_t i = 0;

    while (a[i] != '\0' &&
           b[i] != '\0') {

        if (a[i] != b[i]) {
            return 0;
        }

        i++;
    }

    return a[i] == '\0' &&
           b[i] == '\0';
}

static int string_starts_with(
    const char* string,
    const char* prefix
)
{
    size_t i = 0;

    while (prefix[i] != '\0') {
        if (string[i] != prefix[i]) {
            return 0;
        }

        i++;
    }

    return 1;
}

static void print_number(
    uint64_t value
)
{
    char buffer[32];
    size_t length = 0;

    if (value == 0) {
        print_char('0');
        return;
    }

    while (value != 0) {
        buffer[length++] =
            (char)(
                '0' +
                (value % 10)
            );

        value /= 10;
    }

    while (length > 0) {
        print_char(buffer[--length]);
    }
}

static void print_hex(
    uint64_t value
)
{
    static const char digits[] =
        "0123456789ABCDEF";

    print_str("0x");

    int started = 0;

    for (int shift = 60;
         shift >= 0;
         shift -= 4) {

        uint8_t digit =
            (uint8_t)(
                (value >> shift) & 0xF
            );

        if (digit != 0 ||
            started ||
            shift == 0) {

            print_char(digits[digit]);

            started = 1;
        }
    }
}

static void shell_prompt(void)
{
    print_set_color(
        PRINT_COLOR_LIGHT_GREEN,
        PRINT_COLOR_BLACK
    );

    print_str("root");

    print_set_color(
        PRINT_COLOR_DARK_GRAY,
        PRINT_COLOR_BLACK
    );

    print_str("@");

    print_set_color(
        PRINT_COLOR_LIGHT_BLUE,
        PRINT_COLOR_BLACK
    );

    print_str("fazbear");

    print_set_color(
        PRINT_COLOR_DARK_GRAY,
        PRINT_COLOR_BLACK
    );

    print_str(":");

    print_set_color(
        PRINT_COLOR_WHITE,
        PRINT_COLOR_BLACK
    );

    print_str("/");

    print_set_color(
        PRINT_COLOR_DARK_GRAY,
        PRINT_COLOR_BLACK
    );

    print_str("> ");

    print_set_color(
        PRINT_COLOR_LIGHT_GRAY,
        PRINT_COLOR_BLACK
    );
}

static void command_help(void)
{
    const char* lines[] = {
        "",
        "FazbearOS command reference",
        "============================",
        "",
        "SYSTEM",
        "  help       Show command reference",
        "  clear      Clear the terminal",
        "  version    Show OS version",
        "  uptime     Show system uptime",
        "  cpu        Show processor information",
        "  date       Show system date/time",
        "  reboot     Restart the machine",
        "  shutdown   Halt the machine",
        "",
        "TERMINAL",
        "  echo       Print text",
        "  sleep      Wait for milliseconds",
        "",
        "Type a command followed by ENTER.",
        ""
    };

    size_t line_count =
        sizeof(lines) / sizeof(lines[0]);

    for (size_t i = 0;
         i < line_count;
         i++) {

        print_str(lines[i]);
        print_char('\n');
    }
}

static void command_version(void)
{
    print_str(
        "FazbearOS 0.3.0\n"
        "Architecture: x86_64\n"
        "Kernel:      monolithic\n"
        "Boot:        Multiboot2\n"
        "Interrupts:  IDT + PIC\n"
        "Timer:       PIT\n"
        "Keyboard:    PS/2 IRQ1\n"
        "\n"
    );
}

static void command_uptime(void)
{
    uint64_t ticks = timer_ticks();

    uint64_t seconds =
        timer_uptime_seconds();

    uint64_t hours =
        seconds / 3600;

    uint64_t minutes =
        (seconds % 3600) / 60;

    uint64_t remaining_seconds =
        seconds % 60;

    print_str("Uptime: ");

    print_number(hours);
    print_str("h ");

    print_number(minutes);
    print_str("m ");

    print_number(remaining_seconds);
    print_str("s");

    print_str(" (");

    print_number(ticks);

    print_str(" timer ticks)\n");
}

static void command_cpu(void)
{
    uint32_t eax;
    uint32_t ebx;
    uint32_t ecx;
    uint32_t edx;

    __asm__ volatile (
        "cpuid"
        : "=a"(eax),
          "=b"(ebx),
          "=c"(ecx),
          "=d"(edx)
        : "a"(0)
    );

    char vendor[13];

    vendor[0] =
        (char)(ebx & 0xFF);

    vendor[1] =
        (char)((ebx >> 8) & 0xFF);

    vendor[2] =
        (char)((ebx >> 16) & 0xFF);

    vendor[3] =
        (char)((ebx >> 24) & 0xFF);

    vendor[4] =
        (char)(edx & 0xFF);

    vendor[5] =
        (char)((edx >> 8) & 0xFF);

    vendor[6] =
        (char)((edx >> 16) & 0xFF);

    vendor[7] =
        (char)((edx >> 24) & 0xFF);

    vendor[8] =
        (char)(ecx & 0xFF);

    vendor[9] =
        (char)((ecx >> 8) & 0xFF);

    vendor[10] =
        (char)((ecx >> 16) & 0xFF);

    vendor[11] =
        (char)((ecx >> 24) & 0xFF);

    vendor[12] = '\0';

    print_str("CPU vendor: ");
    print_str(vendor);
    print_char('\n');

    print_str("CPUID max leaf: ");
    print_hex(eax);
    print_char('\n');

    __asm__ volatile (
        "cpuid"
        : "=a"(eax),
          "=b"(ebx),
          "=c"(ecx),
          "=d"(edx)
        : "a"(1)
    );

    print_str("Features:");

    if (edx & (1u << 4)) {
        print_str(" TSC");
    }

    if (edx & (1u << 5)) {
        print_str(" MSR");
    }

    if (edx & (1u << 9)) {
        print_str(" APIC");
    }

    if (edx & (1u << 25)) {
        print_str(" SSE");
    }

    if (edx & (1u << 26)) {
        print_str(" SSE2");
    }

    print_char('\n');
}

static void command_date(void)
{
    struct rtc_datetime datetime;

    rtc_read(&datetime);

    print_number(datetime.year);

    print_char('-');

    if (datetime.month < 10) {
        print_char('0');
    }

    print_number(datetime.month);

    print_char('-');

    if (datetime.day < 10) {
        print_char('0');
    }

    print_number(datetime.day);

    print_char(' ');

    if (datetime.hour < 10) {
        print_char('0');
    }

    print_number(datetime.hour);

    print_char(':');

    if (datetime.minute < 10) {
        print_char('0');
    }

    print_number(datetime.minute);

    print_char(':');

    if (datetime.second < 10) {
        print_char('0');
    }

    print_number(datetime.second);

    print_char('\n');
}

static void command_echo(
    const char* text
)
{
    print_str(text);
    print_char('\n');
}

static void command_sleep(
    const char* argument
)
{
    uint64_t milliseconds = 0;

    if (argument[0] == '\0') {
        print_str(
            "usage: sleep <milliseconds>\n"
        );

        return;
    }

    for (size_t i = 0;
         argument[i] != '\0';
         i++) {

        if (argument[i] < '0' ||
            argument[i] > '9') {

            print_str(
                "usage: sleep <milliseconds>\n"
            );

            return;
        }

        milliseconds =
            milliseconds * 10 +
            (uint64_t)(
                argument[i] - '0'
            );
    }

    print_str("Sleeping for ");

    print_number(milliseconds);

    print_str(" ms...\n");

    timer_sleep(milliseconds);

    print_str("Done.\n");
}

static void command_reboot(void)
{
    print_str(
        "Rebooting...\n"
    );

    keyboard_reboot();
}

static void command_shutdown(void)
{
    print_str(
        "System halted.\n"
    );

    __asm__ volatile ("cli");

    while (1) {
        __asm__ volatile ("hlt");
    }
}

static void execute_command(
    const char* command
)
{
    if (string_equals(command, "")) {
        return;
    }

    if (string_equals(command, "help")) {
        command_help();
        return;
    }

    if (string_equals(command, "clear")) {
        print_clear();
        return;
    }

    if (string_equals(command, "version")) {
        command_version();
        return;
    }

    if (string_equals(command, "uptime")) {
        command_uptime();
        return;
    }

    if (string_equals(command, "cpu")) {
        command_cpu();
        return;
    }

    if (string_equals(command, "date")) {
        command_date();
        return;
    }

    if (string_equals(command, "reboot")) {
        command_reboot();
        return;
    }

    if (string_equals(command, "shutdown")) {
        command_shutdown();
        return;
    }

    if (string_starts_with(command, "echo ")) {
        command_echo(&command[5]);
        return;
    }

    if (string_starts_with(command, "sleep ")) {
        command_sleep(&command[6]);
        return;
    }

    print_set_color(
        PRINT_COLOR_LIGHT_RED,
        PRINT_COLOR_BLACK
    );

    print_str("unknown command: ");

    print_set_color(
        PRINT_COLOR_WHITE,
        PRINT_COLOR_BLACK
    );

    print_str(command);

    print_str(
        "\nType 'help' for a list of commands.\n"
    );

    print_set_color(
        PRINT_COLOR_LIGHT_GRAY,
        PRINT_COLOR_BLACK
    );
}

void shell_start(void)
{
    char command[COMMAND_MAX_LENGTH + 1];

    print_set_color(
        PRINT_COLOR_LIGHT_CYAN,
        PRINT_COLOR_BLACK
    );

    print_str(
        "FazbearOS\n"
        "=========\n"
    );

    print_set_color(
        PRINT_COLOR_LIGHT_GRAY,
        PRINT_COLOR_BLACK
    );

    print_str(
        "Kernel initialized successfully.\n"
        "Hardware interrupts enabled.\n"
        "\n"
    );

    print_str(
        "Type 'help' for available commands.\n"
        "\n"
    );

    while (1) {
        size_t length = 0;

        command[0] = '\0';

        shell_prompt();

        while (1) {
            char character =
                keyboard_get_char();

            if (character == '\n') {
                print_char('\n');

                command[length] = '\0';

                execute_command(command);

                break;
            }

            if (character == '\b') {
                if (length > 0) {
                    length--;

                    command[length] = '\0';

                    print_backspace();
                }

                continue;
            }

            /*
             * The keyboard driver reserves 1-4 for
             * future cursor/navigation handling.
             */
            if (character >= 1 &&
                character <= 4) {
                continue;
            }

            if (character < 32 ||
                character > 126) {
                continue;
            }

            if (length >= COMMAND_MAX_LENGTH) {
                continue;
            }

            command[length++] =
                character;

            command[length] = '\0';

            print_char(character);
        }
    }
}
