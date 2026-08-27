#include "desktop/terminal.h"

#include "graphics.h"
#include "keyboard.h"
#include "heap.h"
#include "rtc.h"
#include "timer.h"

#define TERMINAL_BACKGROUND 0x080C12
#define TERMINAL_PANEL      0x0D131D
#define TERMINAL_BORDER     0x263447
#define TERMINAL_TEXT       0xD7E3F4
#define TERMINAL_GREEN      0x55D6BE
#define TERMINAL_BLUE       0x6FA8FF
#define TERMINAL_RED        0xE05A68
#define TERMINAL_YELLOW     0xE7B84B
#define TERMINAL_MUTED     0x71839D

#define TERMINAL_LEFT       16
#define TERMINAL_TOP        45
#define TERMINAL_RIGHT      16
#define TERMINAL_BOTTOM     16

#define TERMINAL_CHAR_WIDTH  6
#define TERMINAL_CHAR_HEIGHT 7

static int string_equals(
    const char *a,
    const char *b
)
{
    int i = 0;

    if (
        a == 0 ||
        b == 0
    ) {
        return 0;
    }

    while (
        a[i] != '\0' &&
        b[i] != '\0'
    ) {
        if (a[i] != b[i]) {
            return 0;
        }

        i++;
    }

    return
        a[i] == '\0' &&
        b[i] == '\0';
}

static int string_starts_with(
    const char *string,
    const char *prefix
)
{
    int i = 0;

    if (
        string == 0 ||
        prefix == 0
    ) {
        return 0;
    }

    while (prefix[i] != '\0') {
        if (string[i] != prefix[i]) {
            return 0;
        }

        i++;
    }

    return 1;
}

static int string_length(
    const char *string
)
{
    int length = 0;

    if (string == 0) {
        return 0;
    }

    while (string[length] != '\0') {
        length++;
    }

    return length;
}

static void terminal_output_char(
    terminal_t *terminal,
    char character
)
{
    if (terminal == 0) {
        return;
    }

    if (
        terminal->output_length >=
        TERMINAL_OUTPUT_SIZE - 1
    ) {
        /*
         * Drop the oldest line when the terminal
         * output buffer fills.
         */
        int first_newline = -1;

        for (
            int i = 0;
            i < terminal->output_length;
            i++
        ) {
            if (
                terminal->output[i] ==
                '\n'
            ) {
                first_newline = i;
                break;
            }
        }

        if (first_newline >= 0) {
            int remove_count =
                first_newline + 1;

            for (
                int i = remove_count;
                i < terminal->output_length;
                i++
            ) {
                terminal->output[
                    i - remove_count
                ] =
                    terminal->output[i];
            }

            terminal->output_length -=
                remove_count;

            terminal->output[
                terminal->output_length
            ] = '\0';
        } else {
            terminal->output_length = 0;
            terminal->output[0] = '\0';
        }
    }

    terminal->output[
        terminal->output_length
    ] = character;

    terminal->output_length++;

    terminal->output[
        terminal->output_length
    ] = '\0';

    terminal->dirty = true;
}

static void terminal_output(
    terminal_t *terminal,
    const char *string
)
{
    if (
        terminal == 0 ||
        string == 0
    ) {
        return;
    }

    for (
        int i = 0;
        string[i] != '\0';
        i++
    ) {
        terminal_output_char(
            terminal,
            string[i]
        );
    }
}

static void terminal_output_number(
    terminal_t *terminal,
    uint64_t value
)
{
    char buffer[32];
    int length = 0;

    if (value == 0) {
        terminal_output_char(
            terminal,
            '0'
        );

        return;
    }

    while (
        value != 0 &&
        length < 31
    ) {
        buffer[length++] =
            (char)(
                '0' +
                (value % 10)
            );

        value /= 10;
    }

    while (length > 0) {
        length--;

        terminal_output_char(
            terminal,
            buffer[length]
        );
    }
}

static void terminal_output_hex(
    terminal_t *terminal,
    uint64_t value
)
{
    static const char digits[] =
        "0123456789ABCDEF";

    terminal_output(
        terminal,
        "0x"
    );

    int started = 0;

    for (
        int shift = 60;
        shift >= 0;
        shift -= 4
    ) {
        uint8_t digit =
            (uint8_t)(
                (value >> shift) &
                0xF
            );

        if (
            digit != 0 ||
            started ||
            shift == 0
        ) {
            terminal_output_char(
                terminal,
                digits[digit]
            );

            started = 1;
        }
    }
}

static void terminal_output_prompt(
    terminal_t *terminal
)
{
    terminal_output(
        terminal,
        "root@fazbear:/ > "
    );
}

static void terminal_output_prompt_prefix(
    terminal_t *terminal
)
{
    terminal_output(
        terminal,
        "\nroot@fazbear:/ > "
    );
}

static void terminal_clear_output(
    terminal_t *terminal
)
{
    if (terminal == 0) {
        return;
    }

    terminal->output_length = 0;
    terminal->output[0] = '\0';

    terminal->dirty = true;
}

static void terminal_history_add(
    terminal_t *terminal
)
{
    if (
        terminal == 0 ||
        terminal->input_length == 0
    ) {
        return;
    }

    /*
     * Don't add an identical command twice in a row.
     */
    if (
        terminal->history_count > 0 &&
        string_equals(
            terminal->history[
                terminal->history_count - 1
            ],
            terminal->input
        )
    ) {
        terminal->history_position =
            terminal->history_count;

        return;
    }

    if (
        terminal->history_count <
        TERMINAL_HISTORY_SIZE
    ) {
        int index =
            terminal->history_count;

        for (
            int i = 0;
            i < TERMINAL_INPUT_SIZE;
            i++
        ) {
            terminal->history[index][i] =
                terminal->input[i];

            if (
                terminal->input[i] ==
                '\0'
            ) {
                break;
            }
        }

        terminal->history_count++;
    } else {
        for (
            int i = 1;
            i < TERMINAL_HISTORY_SIZE;
            i++
        ) {
            for (
                int j = 0;
                j < TERMINAL_INPUT_SIZE;
                j++
            ) {
                terminal->history[i - 1][j] =
                    terminal->history[i][j];
            }
        }

        for (
            int j = 0;
            j < TERMINAL_INPUT_SIZE;
            j++
        ) {
            terminal->history[
                TERMINAL_HISTORY_SIZE - 1
            ][j] =
                terminal->input[j];

            if (
                terminal->input[j] ==
                '\0'
            ) {
                break;
            }
        }
    }

    terminal->history_position =
        terminal->history_count;
}

static void terminal_set_input(
    terminal_t *terminal,
    const char *text
)
{
    if (
        terminal == 0 ||
        text == 0
    ) {
        return;
    }

    terminal->input_length = 0;

    while (
        text[
            terminal->input_length
        ] != '\0' &&
        terminal->input_length <
            TERMINAL_INPUT_SIZE - 1
    ) {
        terminal->input[
            terminal->input_length
        ] =
            text[
                terminal->input_length
            ];

        terminal->input_length++;
    }

    terminal->input[
        terminal->input_length
    ] = '\0';

    terminal->dirty = true;
}

static void terminal_history_previous(
    terminal_t *terminal
)
{
    if (
        terminal == 0 ||
        terminal->history_count == 0
    ) {
        return;
    }

    if (
        terminal->history_position > 0
    ) {
        terminal->history_position--;
    }

    terminal_set_input(
        terminal,
        terminal->history[
            terminal->history_position
        ]
    );
}

static void terminal_history_next(
    terminal_t *terminal
)
{
    if (
        terminal == 0 ||
        terminal->history_count == 0
    ) {
        return;
    }

    if (
        terminal->history_position <
        terminal->history_count
    ) {
        terminal->history_position++;
    }

    if (
        terminal->history_position >=
        terminal->history_count
    ) {
        terminal_set_input(
            terminal,
            ""
        );

        terminal->history_position =
            terminal->history_count;

        return;
    }

    terminal_set_input(
        terminal,
        terminal->history[
            terminal->history_position
        ]
    );
}

static void terminal_command_help(
    terminal_t *terminal
)
{
    terminal_output(
        terminal,
        "\nFazbearOS Terminal\n"
        "==================\n"
        "\n"
        "SYSTEM\n"
        "  help       Show this help\n"
        "  clear      Clear terminal output\n"
        "  version    Show OS version\n"
        "  uname      Show system information\n"
        "  uptime     Show system uptime\n"
        "  cpu        Show processor information\n"
        "  mem        Show heap memory usage\n"
        "  date       Show system date/time\n"
        "  whoami     Show current user\n"
        "\n"
        "COMMANDS\n"
        "  echo TEXT  Print text\n"
        "  sleep MS   Wait for milliseconds\n"
        "\n"
        "POWER\n"
        "  reboot     Restart FazbearOS\n"
        "  shutdown   Halt the system\n"
        "\n"
        "Use UP/DOWN for command history.\n"
    );
}

static void terminal_command_version(
    terminal_t *terminal
)
{
    terminal_output(
        terminal,
        "\nFazbearOS 0.4.0\n"
        "Architecture: x86_64\n"
        "Kernel:      monolithic\n"
        "Boot:        Multiboot2\n"
        "Desktop:     graphical\n"
        "Keyboard:    PS/2 IRQ1\n"
        "Mouse:       PS/2\n"
        "Filesystem:  RAMFS/VFS\n"
    );
}

static void terminal_command_uname(
    terminal_t *terminal
)
{
    terminal_output(
        terminal,
        "FazbearOS fazbear 0.4.0 x86_64\n"
    );
}

static void terminal_command_whoami(
    terminal_t *terminal
)
{
    terminal_output(
        terminal,
        "root\n"
    );
}

static void terminal_command_uptime(
    terminal_t *terminal
)
{
    uint64_t ticks =
        timer_ticks();

    uint64_t seconds =
        timer_uptime_seconds();

    uint64_t hours =
        seconds / 3600;

    uint64_t minutes =
        (seconds % 3600) / 60;

    uint64_t remaining_seconds =
        seconds % 60;

    terminal_output(
        terminal,
        "Uptime: "
    );

    terminal_output_number(
        terminal,
        hours
    );

    terminal_output(
        terminal,
        "h "
    );

    terminal_output_number(
        terminal,
        minutes
    );

    terminal_output(
        terminal,
        "m "
    );

    terminal_output_number(
        terminal,
        remaining_seconds
    );

    terminal_output(
        terminal,
        "s ("
    );

    terminal_output_number(
        terminal,
        ticks
    );

    terminal_output(
        terminal,
        " timer ticks)\n"
    );
}

static void terminal_command_mem(
    terminal_t *terminal
)
{
    size_t used =
        heap_used();

    size_t free_memory =
        heap_free();

    terminal_output(
        terminal,
        "Heap used: "
    );

    terminal_output_number(
        terminal,
        used
    );

    terminal_output(
        terminal,
        " bytes\n"
    );

    terminal_output(
        terminal,
        "Heap free: "
    );

    terminal_output_number(
        terminal,
        free_memory
    );

    terminal_output(
        terminal,
        " bytes\n"
    );

    terminal_output(
        terminal,
        "Heap total: "
    );

    terminal_output_number(
        terminal,
        used + free_memory
    );

    terminal_output(
        terminal,
        " bytes\n"
    );
}

static void terminal_command_cpu(
    terminal_t *terminal
)
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

    terminal_output(
        terminal,
        "CPU vendor: "
    );

    terminal_output(
        terminal,
        vendor
    );

    terminal_output(
        terminal,
        "\nCPUID max leaf: "
    );

    terminal_output_hex(
        terminal,
        eax
    );

    terminal_output(
        terminal,
        "\nFeatures:"
    );

    __asm__ volatile (
        "cpuid"
        : "=a"(eax),
          "=b"(ebx),
          "=c"(ecx),
          "=d"(edx)
        : "a"(1)
    );

    if (edx & (1u << 4)) {
        terminal_output(
            terminal,
            " TSC"
        );
    }

    if (edx & (1u << 5)) {
        terminal_output(
            terminal,
            " MSR"
        );
    }

    if (edx & (1u << 9)) {
        terminal_output(
            terminal,
            " APIC"
        );
    }

    if (edx & (1u << 25)) {
        terminal_output(
            terminal,
            " SSE"
        );
    }

    if (edx & (1u << 26)) {
        terminal_output(
            terminal,
            " SSE2"
        );
    }

    terminal_output_char(
        terminal,
        '\n'
    );
}

static void terminal_command_date(
    terminal_t *terminal
)
{
    struct rtc_datetime datetime;

    rtc_read(
        &datetime
    );

    terminal_output_number(
        terminal,
        datetime.year
    );

    terminal_output_char(
        terminal,
        '-'
    );

    if (datetime.month < 10) {
        terminal_output_char(
            terminal,
            '0'
        );
    }

    terminal_output_number(
        terminal,
        datetime.month
    );

    terminal_output_char(
        terminal,
        '-'
    );

    if (datetime.day < 10) {
        terminal_output_char(
            terminal,
            '0'
        );
    }

    terminal_output_number(
        terminal,
        datetime.day
    );

    terminal_output_char(
        terminal,
        ' '
    );

    if (datetime.hour < 10) {
        terminal_output_char(
            terminal,
            '0'
        );
    }

    terminal_output_number(
        terminal,
        datetime.hour
    );

    terminal_output_char(
        terminal,
        ':'
    );

    if (datetime.minute < 10) {
        terminal_output_char(
            terminal,
            '0'
        );
    }

    terminal_output_number(
        terminal,
        datetime.minute
    );

    terminal_output_char(
        terminal,
        ':'
    );

    if (datetime.second < 10) {
        terminal_output_char(
            terminal,
            '0'
        );
    }

    terminal_output_number(
        terminal,
        datetime.second
    );

    terminal_output_char(
        terminal,
        '\n'
    );
}

static int terminal_parse_milliseconds(
    const char *argument,
    uint64_t *result
)
{
    if (
        argument == 0 ||
        result == 0 ||
        argument[0] == '\0'
    ) {
        return 0;
    }

    uint64_t value = 0;

    for (
        int i = 0;
        argument[i] != '\0';
        i++
    ) {
        if (
            argument[i] < '0' ||
            argument[i] > '9'
        ) {
            return 0;
        }

        value =
            value * 10 +
            (uint64_t)(
                argument[i] - '0'
            );
    }

    *result = value;

    return 1;
}

static void terminal_command_sleep(
    terminal_t *terminal,
    const char *argument
)
{
    uint64_t milliseconds = 0;

    if (
        !terminal_parse_milliseconds(
            argument,
            &milliseconds
        )
    ) {
        terminal_output(
            terminal,
            "usage: sleep <milliseconds>\n"
        );

        return;
    }

    terminal_output(
        terminal,
        "Sleeping for "
    );

    terminal_output_number(
        terminal,
        milliseconds
    );

    terminal_output(
        terminal,
        " ms...\n"
    );

    timer_sleep(
        milliseconds
    );

    terminal_output(
        terminal,
        "Done.\n"
    );
}

static void terminal_command_reboot(
    terminal_t *terminal
)
{
    terminal_output(
        terminal,
        "Rebooting...\n"
    );

    keyboard_reboot();
}

static void terminal_command_shutdown(
    terminal_t *terminal
)
{
    terminal_output(
        terminal,
        "System halted.\n"
    );

    __asm__ volatile ("cli");

    while (1) {
        __asm__ volatile ("hlt");
    }
}

static void terminal_execute(
    terminal_t *terminal,
    const char *command
)
{
    if (
        terminal == 0 ||
        command == 0
    ) {
        return;
    }

    if (
        string_equals(
            command,
            ""
        )
    ) {
        return;
    }

    if (
        string_equals(
            command,
            "help"
        )
    ) {
        terminal_command_help(
            terminal
        );

        return;
    }

    if (
        string_equals(
            command,
            "clear"
        )
    ) {
        terminal_clear_output(
            terminal
        );

        return;
    }

    if (
        string_equals(
            command,
            "version"
        )
    ) {
        terminal_command_version(
            terminal
        );

        return;
    }

    if (
        string_equals(
            command,
            "uname"
        )
    ) {
        terminal_command_uname(
            terminal
        );

        return;
    }

    if (
        string_equals(
            command,
            "uptime"
        )
    ) {
        terminal_command_uptime(
            terminal
        );

        return;
    }

    if (
        string_equals(
            command,
            "cpu"
        )
    ) {
        terminal_command_cpu(
            terminal
        );

        return;
    }

    if (
        string_equals(
            command,
            "mem"
        )
    ) {
        terminal_command_mem(
            terminal
        );

        return;
    }

    if (
        string_equals(
            command,
            "date"
        )
    ) {
        terminal_command_date(
            terminal
        );

        return;
    }

    if (
        string_equals(
            command,
            "whoami"
        )
    ) {
        terminal_command_whoami(
            terminal
        );

        return;
    }

    if (
        string_equals(
            command,
            "reboot"
        )
    ) {
        terminal_command_reboot(
            terminal
        );

        return;
    }

    if (
        string_equals(
            command,
            "shutdown"
        )
    ) {
        terminal_command_shutdown(
            terminal
        );

        return;
    }

    if (
        string_starts_with(
            command,
            "echo "
        )
    ) {
        terminal_output(
            terminal,
            &command[5]
        );

        terminal_output_char(
            terminal,
            '\n'
        );

        return;
    }

    if (
        string_equals(
            command,
            "echo"
        )
    ) {
        terminal_output_char(
            terminal,
            '\n'
        );

        return;
    }

    if (
        string_starts_with(
            command,
            "sleep "
        )
    ) {
        terminal_command_sleep(
            terminal,
            &command[6]
        );

        return;
    }

    if (
        string_starts_with(
            command,
            "sleep"
        ) &&
        command[5] == '\0'
    ) {
        terminal_output(
            terminal,
            "usage: sleep <milliseconds>\n"
        );

        return;
    }

    terminal_output(
        terminal,
        "unknown command: "
    );

    terminal_output(
        terminal,
        command
    );

    terminal_output(
        terminal,
        "\nType 'help' for a list of commands.\n"
    );
}

static void terminal_submit(
    terminal_t *terminal
)
{
    if (terminal == 0) {
        return;
    }

    /*
     * The prompt is rendered from output, so put the
     * current command into the terminal transcript.
     */
    terminal_output(
        terminal,
        terminal->input
    );

    terminal_output_char(
        terminal,
        '\n'
    );

    terminal_history_add(
        terminal
    );

    terminal_execute(
        terminal,
        terminal->input
    );

    terminal->input_length = 0;
    terminal->input[0] = '\0';

    terminal_output_prompt_prefix(
        terminal
    );

    terminal->dirty = true;
}

void terminal_init(
    terminal_t *terminal
)
{
    if (terminal == 0) {
        return;
    }

    for (
        int i = 0;
        i < TERMINAL_OUTPUT_SIZE;
        i++
    ) {
        terminal->output[i] = '\0';
    }

    for (
        int i = 0;
        i < TERMINAL_INPUT_SIZE;
        i++
    ) {
        terminal->input[i] = '\0';
    }

    for (
        int i = 0;
        i < TERMINAL_HISTORY_SIZE;
        i++
    ) {
        for (
            int j = 0;
            j < TERMINAL_INPUT_SIZE;
            j++
        ) {
            terminal->history[i][j] =
                '\0';
        }
    }

    terminal->output_length = 0;
    terminal->input_length = 0;

    terminal->history_count = 0;
    terminal->history_position = 0;

    terminal->active = true;
    terminal->dirty = true;

    terminal_output(
        terminal,
        "FazbearOS Terminal\n"
        "==================\n"
        "Graphical command shell ready.\n"
        "Type 'help' for available commands.\n"
    );

    terminal_output_prompt(
        terminal
    );
}

void terminal_handle_key(
    terminal_t *terminal,
    char character
)
{
    if (
        terminal == 0 ||
        !terminal->active
    ) {
        return;
    }

    /*
     * Up arrow.
     */
    if (character == '\x01') {
        terminal_history_previous(
            terminal
        );

        return;
    }

    /*
     * Down arrow.
     */
    if (character == '\x02') {
        terminal_history_next(
            terminal
        );

        return;
    }

    /*
     * Left/right arrows are intentionally ignored for
     * this first shell. The command line remains append-only,
     * which keeps editing deterministic until a full cursor
     * model is added.
     */
    if (
        character == '\x03' ||
        character == '\x04'
    ) {
        return;
    }

    /*
     * Enter.
     */
    if (character == '\n') {
        terminal_submit(
            terminal
        );

        return;
    }

    /*
     * Backspace.
     */
    if (character == '\b') {
        if (
            terminal->input_length > 0
        ) {
            terminal->input_length--;

            terminal->input[
                terminal->input_length
            ] = '\0';

            terminal->dirty = true;
        }

        return;
    }

    /*
     * Tab is four spaces.
     */
    if (character == '\t') {
        for (int i = 0; i < 4; i++) {
            if (
                terminal->input_length >=
                TERMINAL_INPUT_SIZE - 1
            ) {
                break;
            }

            terminal->input[
                terminal->input_length++
            ] = ' ';
        }

        terminal->input[
            terminal->input_length
        ] = '\0';

        terminal->dirty = true;

        return;
    }

    if (
        character < 32 ||
        character > 126
    ) {
        return;
    }

    if (
        terminal->input_length >=
        TERMINAL_INPUT_SIZE - 1
    ) {
        return;
    }

    terminal->input[
        terminal->input_length++
    ] = character;

    terminal->input[
        terminal->input_length
    ] = '\0';

    terminal->dirty = true;
}

void terminal_update(
    terminal_t *terminal
)
{
    if (
        terminal == 0 ||
        !terminal->active
    ) {
        return;
    }

    while (
        keyboard_available()
    ) {
        terminal_handle_key(
            terminal,
            keyboard_get_char()
        );
    }
}

static int terminal_count_visual_lines(
    const terminal_t *terminal,
    const window_t *window
)
{
    if (
        terminal == 0 ||
        window == 0
    ) {
        return 1;
    }

    int left =
        window->x +
        TERMINAL_LEFT;

    int right =
        window->x +
        window->width -
        TERMINAL_RIGHT;

    int width =
        right - left;

    if (width < TERMINAL_CHAR_WIDTH) {
        width = TERMINAL_CHAR_WIDTH;
    }

    int chars_per_line =
        width /
        TERMINAL_CHAR_WIDTH;

    if (chars_per_line < 1) {
        chars_per_line = 1;
    }

    int lines = 1;
    int column = 0;

    for (
        int i = 0;
        i < terminal->output_length;
        i++
    ) {
        char character =
            terminal->output[i];

        if (character == '\n') {
            lines++;
            column = 0;
            continue;
        }

        if (
            column >=
            chars_per_line
        ) {
            lines++;
            column = 0;
        }

        column++;
    }

    /*
     * Input line is another visual line.
     */
    for (
        int i = 0;
        i < terminal->input_length;
        i++
    ) {
        if (
            column >=
            chars_per_line
        ) {
            lines++;
            column = 0;
        }

        column++;
    }

    return lines;
}

static void terminal_render_line(
    const char *text,
    int length,
    int *cursor_x,
    int *cursor_y,
    int left,
    int right,
    int bottom,
    uint32_t foreground,
    uint32_t background
)
{
    if (
        text == 0 ||
        cursor_x == 0 ||
        cursor_y == 0
    ) {
        return;
    }

    for (
        int i = 0;
        i < length;
        i++
    ) {
        char character =
            text[i];

        if (
            character == '\n'
        ) {
            *cursor_x = left;
            *cursor_y +=
                TERMINAL_CHAR_HEIGHT;

            continue;
        }

        if (
            *cursor_x +
            TERMINAL_CHAR_WIDTH >
            right
        ) {
            *cursor_x = left;
            *cursor_y +=
                TERMINAL_CHAR_HEIGHT;
        }

        if (
            *cursor_y +
            TERMINAL_CHAR_HEIGHT >
            bottom
        ) {
            return;
        }

        graphics_draw_char(
            *cursor_x,
            *cursor_y,
            character,
            foreground,
            background,
            1
        );

        *cursor_x +=
            TERMINAL_CHAR_WIDTH;
    }
}

void terminal_render(
    const terminal_t *terminal,
    const window_t *window
)
{
    if (
        terminal == 0 ||
        window == 0 ||
        !window->visible ||
        window->minimized
    ) {
        return;
    }

    int content_width =
        window->width - 20;

    int content_height =
        window->height - 42;

    if (content_width < 30) {
        content_width = 30;
    }

    if (content_height < 30) {
        content_height = 30;
    }

    int left =
        window->x +
        TERMINAL_LEFT;

    int top =
        window->y +
        TERMINAL_TOP;

    int right =
        window->x +
        window->width -
        TERMINAL_RIGHT;

    int bottom =
        window->y +
        window->height -
        TERMINAL_BOTTOM;

    /*
     * Terminal background.
     */
    graphics_fill_rect(
        window->x + 10,
        window->y + 38,
        content_width,
        content_height,
        TERMINAL_BACKGROUND
    );

    graphics_rect(
        window->x + 10,
        window->y + 38,
        content_width,
        content_height,
        TERMINAL_BORDER
    );

    /*
     * Header inside the terminal.
     */
    graphics_draw_text(
        window->x + 17,
        window->y + 42,
        "TERMINAL",
        TERMINAL_GREEN,
        TERMINAL_BACKGROUND,
        1
    );

    graphics_draw_text(
        window->x + 72,
        window->y + 42,
        "FazbearOS shell",
        TERMINAL_MUTED,
        TERMINAL_BACKGROUND,
        1
    );

    /*
     * Determine how many visual lines fit.
     */
    int available_height =
        bottom - top;

    int visible_lines =
        available_height /
        TERMINAL_CHAR_HEIGHT;

    if (visible_lines < 1) {
        visible_lines = 1;
    }

    int total_lines =
        terminal_count_visual_lines(
            terminal,
            window
        );

    int skip_lines =
        total_lines -
        visible_lines;

    if (skip_lines < 0) {
        skip_lines = 0;
    }

    int cursor_x = left;
    int cursor_y = top;

    int current_line = 0;

    /*
     * Render the output while skipping lines that no longer
     * fit at the top.
     */
    int output_line_start = 0;

    for (
        int i = 0;
        i <= terminal->output_length;
        i++
    ) {
        if (
            i == terminal->output_length ||
            terminal->output[i] == '\n'
        ) {
            int line_length =
                i - output_line_start;

            if (
                current_line >=
                skip_lines
            ) {
                terminal_render_line(
                    &terminal->output[
                        output_line_start
                    ],
                    line_length,
                    &cursor_x,
                    &cursor_y,
                    left,
                    right,
                    bottom,
                    TERMINAL_TEXT,
                    TERMINAL_BACKGROUND
                );

                /*
                 * The line renderer can wrap. Determine the
                 * next position by simulating the same width.
                 */
                int chars_per_line =
                    (right - left) /
                    TERMINAL_CHAR_WIDTH;

                if (chars_per_line < 1) {
                    chars_per_line = 1;
                }

                if (
                    line_length >
                    chars_per_line
                ) {
                    int wrapped =
                        line_length /
                        chars_per_line;

                    if (
                        line_length %
                        chars_per_line ==
                        0
                    ) {
                        wrapped--;
                    }

                    cursor_y +=
                        wrapped *
                        TERMINAL_CHAR_HEIGHT;

                    cursor_x =
                        left +
                        (
                            line_length %
                            chars_per_line
                        ) *
                        TERMINAL_CHAR_WIDTH;

                    if (
                        line_length %
                        chars_per_line ==
                        0
                    ) {
                        cursor_x = left;
                    }
                }

                cursor_x = left;
                cursor_y +=
                    TERMINAL_CHAR_HEIGHT;
            }

            current_line++;

            output_line_start =
                i + 1;
        }
    }

    /*
     * Render the current command prompt/input line.
     */
    if (
        current_line >= skip_lines
    ) {
        const char prompt[] =
            "root@fazbear:/ > ";

        int prompt_length =
            string_length(prompt);

        terminal_render_line(
            prompt,
            prompt_length,
            &cursor_x,
            &cursor_y,
            left,
            right,
            bottom,
            TERMINAL_GREEN,
            TERMINAL_BACKGROUND
        );

        terminal_render_line(
            terminal->input,
            terminal->input_length,
            &cursor_x,
            &cursor_y,
            left,
            right,
            bottom,
            TERMINAL_TEXT,
            TERMINAL_BACKGROUND
        );

        /*
         * Visible input cursor.
         */
        if (
            cursor_y +
            TERMINAL_CHAR_HEIGHT <=
            bottom
        ) {
            graphics_fill_rect(
                cursor_x,
                cursor_y,
                2,
                6,
                TERMINAL_GREEN
            );
        }
    }
}
