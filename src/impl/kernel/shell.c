#include "shell.h"
#include "print.h"
#include "keyboard.h"

#define COMMAND_MAX_LENGTH 55

static int string_equals(const char* a, const char* b)
{
    size_t i = 0;

    while (a[i] != '\0' && b[i] != '\0') {
        if (a[i] != b[i]) {
            return 0;
        }

        i++;
    }

    return a[i] == '\0' && b[i] == '\0';
}

static void print_banner(void)
{
    print_clear();

    print_set_color(
        PRINT_COLOR_RED,
        PRINT_COLOR_BLACK
    );

    print_str(
        "============================================================\n"
        "                 FAZBEAR ENTERTAINMENT\n"
        "                    FAZBEAR OS v0.1\n"
        "============================================================\n"
    );

    print_set_color(
        PRINT_COLOR_LIGHT_GRAY,
        PRINT_COLOR_BLACK
    );

    print_str(
        "\n"
        "Welcome, night employee.\n"
        "\n"
    );

    print_set_color(
        PRINT_COLOR_YELLOW,
        PRINT_COLOR_BLACK
    );

    print_str(
        "NOTICE: The animatronic control systems are currently\n"
        "operating in DEVELOPMENT MODE.\n"
        "\n"
    );

    print_set_color(
        PRINT_COLOR_LIGHT_GREEN,
        PRINT_COLOR_BLACK
    );

    print_str(
        "Type 'help' for available commands.\n"
        "\n"
    );

    print_set_color(
        PRINT_COLOR_WHITE,
        PRINT_COLOR_BLACK
    );
}

static void print_prompt(void)
{
    print_set_color(
        PRINT_COLOR_RED,
        PRINT_COLOR_BLACK
    );

    print_str("fazbear");

    print_set_color(
        PRINT_COLOR_DARK_GRAY,
        PRINT_COLOR_BLACK
    );

    print_str("@");

    print_set_color(
        PRINT_COLOR_YELLOW,
        PRINT_COLOR_BLACK
    );

    print_str("security");

    print_set_color(
        PRINT_COLOR_DARK_GRAY,
        PRINT_COLOR_BLACK
    );

    print_str(":");

    print_set_color(
        PRINT_COLOR_LIGHT_BLUE,
        PRINT_COLOR_BLACK
    );

    print_str("/");

    print_set_color(
        PRINT_COLOR_WHITE,
        PRINT_COLOR_BLACK
    );

    print_str("> ");
}

static void command_help(void)
{
    print_set_color(
        PRINT_COLOR_YELLOW,
        PRINT_COLOR_BLACK
    );

    print_str("FAZBEAR OS COMMAND DIRECTORY\n\n");

    print_set_color(
        PRINT_COLOR_WHITE,
        PRINT_COLOR_BLACK
    );

    print_str(
        "  help       Show this command list\n"
        "  clear      Clear the terminal\n"
        "  about      Display system information\n"
        "  status     Display facility status\n"
        "  lore       Display a restricted message\n"
        "  night      Start Night Shift mode\n"
        "  freddy     Check Freddy Fazbear\n"
        "  bonnie     Check Bonnie\n"
        "  chica      Check Chica\n"
        "  foxy       Check Foxy\n"
        "  reboot     Restart the machine\n"
        "\n"
    );
}

static void command_about(void)
{
    print_set_color(
        PRINT_COLOR_LIGHT_CYAN,
        PRINT_COLOR_BLACK
    );

    print_str("FAZBEAR OS\n\n");

    print_set_color(
        PRINT_COLOR_WHITE,
        PRINT_COLOR_BLACK
    );

    print_str(
        "Name:        FazbearOS\n"
        "Version:     0.1.0-nightshift\n"
        "Architecture:x86_64\n"
        "Kernel:      Fazbear Kernel\n"
        "Boot:        Multiboot2\n"
        "Display:     VGA Text Mode\n"
        "Input:       PS/2 Keyboard\n"
        "Memory:      Early bootstrap paging\n"
        "\n"
        "Purpose:     Entertainment and security systems\n"
        "Status:      EXTREMELY EXPERIMENTAL\n"
        "\n"
    );
}

static void command_status(void)
{
    print_set_color(
        PRINT_COLOR_LIGHT_GREEN,
        PRINT_COLOR_BLACK
    );

    print_str("FACILITY STATUS\n\n");

    print_set_color(
        PRINT_COLOR_WHITE,
        PRINT_COLOR_BLACK
    );

    print_str(
        "Power systems .............. ONLINE\n"
        "Main terminal .............. ONLINE\n"
        "Security network ........... ONLINE\n"
        "Camera network ............. STANDBY\n"
        "Door control ............... STANDBY\n"
        "Animatronic controller ..... ACTIVE\n"
        "Night shift protocol ....... ARMED\n"
        "\n"
    );

    print_set_color(
        PRINT_COLOR_RED,
        PRINT_COLOR_BLACK
    );

    print_str(
        "WARNING: Motion detected in restricted area.\n"
        "\n"
    );

    print_set_color(
        PRINT_COLOR_WHITE,
        PRINT_COLOR_BLACK
    );
}

static void command_lore(void)
{
    print_set_color(
        PRINT_COLOR_RED,
        PRINT_COLOR_BLACK
    );

    print_str("RESTRICTED FILE\n\n");

    print_set_color(
        PRINT_COLOR_LIGHT_GRAY,
        PRINT_COLOR_BLACK
    );

    print_str(
        "The restaurant was supposed to be closed.\n"
        "\n"
        "The cameras disagree.\n"
        "\n"
        "Management has requested that employees do not\n"
        "investigate unusual movement after midnight.\n"
        "\n"
        "If an animatronic is standing somewhere it should\n"
        "not be standing, do not approach it.\n"
        "\n"
        "If you hear music, check the cameras.\n"
        "\n"
        "If the cameras stop working...\n"
        "\n"
    );

    print_set_color(
        PRINT_COLOR_RED,
        PRINT_COLOR_BLACK
    );

    print_str(
        "DO NOT LEAVE THE OFFICE.\n"
        "\n"
    );

    print_set_color(
        PRINT_COLOR_WHITE,
        PRINT_COLOR_BLACK
    );
}

static void command_freddy(void)
{
    print_set_color(
        PRINT_COLOR_BROWN,
        PRINT_COLOR_BLACK
    );

    print_str(
        "FREDDY FAZBEAR\n\n"
    );

    print_set_color(
        PRINT_COLOR_WHITE,
        PRINT_COLOR_BLACK
    );

    print_str(
        "Location: UNKNOWN\n"
        "Status:   ACTIVE\n"
        "Threat:   ???\n"
        "\n"
        "Freddy is currently not visible on the office cameras.\n"
        "\n"
    );
}

static void command_bonnie(void)
{
    print_set_color(
        PRINT_COLOR_LIGHT_BLUE,
        PRINT_COLOR_BLACK
    );

    print_str(
        "BONNIE\n\n"
    );

    print_set_color(
        PRINT_COLOR_WHITE,
        PRINT_COLOR_BLACK
    );

    print_str(
        "Location: WEST HALL\n"
        "Status:   MOVING\n"
        "Threat:   HIGH\n"
        "\n"
        "Recommendation: Monitor the left door.\n"
        "\n"
    );
}

static void command_chica(void)
{
    print_set_color(
        PRINT_COLOR_YELLOW,
        PRINT_COLOR_BLACK
    );

    print_str(
        "CHICA\n\n"
    );

    print_set_color(
        PRINT_COLOR_WHITE,
        PRINT_COLOR_BLACK
    );

    print_str(
        "Location: EAST HALL\n"
        "Status:   ACTIVE\n"
        "Threat:   HIGH\n"
        "\n"
        "Recommendation: Monitor the right door.\n"
        "\n"
    );
}

static void command_foxy(void)
{
    print_set_color(
        PRINT_COLOR_LIGHT_RED,
        PRINT_COLOR_BLACK
    );

    print_str(
        "FOXY\n\n"
    );

    print_set_color(
        PRINT_COLOR_WHITE,
        PRINT_COLOR_BLACK
    );

    print_str(
        "Location: PIRATE COVE\n"
        "Status:   UNKNOWN\n"
        "Threat:   CRITICAL\n"
        "\n"
        "Recommendation: Check Pirate Cove immediately.\n"
        "\n"
    );
}

static void command_night(void)
{
    print_clear();

    print_set_color(
        PRINT_COLOR_RED,
        PRINT_COLOR_BLACK
    );

    print_str(
        "************************************************************\n"
        "*                      NIGHT SHIFT                          *\n"
        "************************************************************\n"
        "\n"
    );

    print_set_color(
        PRINT_COLOR_WHITE,
        PRINT_COLOR_BLACK
    );

    print_str(
        "12:00 AM\n"
        "\n"
        "The restaurant is closed.\n"
        "You are alone in the security office.\n"
        "\n"
    );

    print_set_color(
        PRINT_COLOR_YELLOW,
        PRINT_COLOR_BLACK
    );

    print_str(
        "POWER: 100%\n"
        "\n"
    );

    print_set_color(
        PRINT_COLOR_LIGHT_GREEN,
        PRINT_COLOR_BLACK
    );

    print_str(
        "CAMERAS: ONLINE\n"
        "DOORS: CLOSED\n"
        "\n"
    );

    print_set_color(
        PRINT_COLOR_RED,
        PRINT_COLOR_BLACK
    );

    print_str(
        "Something moved.\n"
        "\n"
    );

    print_set_color(
        PRINT_COLOR_WHITE,
        PRINT_COLOR_BLACK
    );

    print_str(
        "Night Shift mode is only a preview for now.\n"
        "The real security system is coming.\n"
        "\n"
    );
}

static void execute_command(const char* command)
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

    if (string_equals(command, "about")) {
        command_about();
        return;
    }

    if (string_equals(command, "status")) {
        command_status();
        return;
    }

    if (string_equals(command, "lore")) {
        command_lore();
        return;
    }

    if (string_equals(command, "night")) {
        command_night();
        return;
    }

    if (string_equals(command, "freddy")) {
        command_freddy();
        return;
    }

    if (string_equals(command, "bonnie")) {
        command_bonnie();
        return;
    }

    if (string_equals(command, "chica")) {
        command_chica();
        return;
    }

    if (string_equals(command, "foxy")) {
        command_foxy();
        return;
    }

    if (string_equals(command, "reboot")) {
        print_set_color(
            PRINT_COLOR_RED,
            PRINT_COLOR_BLACK
        );

        print_str(
            "Rebooting FazbearOS...\n"
        );

        keyboard_reboot();
        return;
    }

    print_set_color(
        PRINT_COLOR_LIGHT_RED,
        PRINT_COLOR_BLACK
    );

    print_str("fazbear: command not found: ");

    print_set_color(
        PRINT_COLOR_WHITE,
        PRINT_COLOR_BLACK
    );

    print_str(command);

    print_str(
        "\n"
        "Type 'help' to see available commands.\n"
        "\n"
    );
}

void shell_start(void)
{
    char command[COMMAND_MAX_LENGTH + 1];
    size_t length;

    print_banner();

    while (1) {
        length = 0;
        command[0] = '\0';

        print_prompt();

        while (1) {
            char character = keyboard_get_char();

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

            if (character == '\t') {
                continue;
            }

            if (character < 32 || character > 126) {
                continue;
            }

            if (length >= COMMAND_MAX_LENGTH) {
                continue;
            }

            command[length] = character;
            length++;

            command[length] = '\0';

            print_char(character);
        }
    }
}
