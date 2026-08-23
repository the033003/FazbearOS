#include "log.h"

#include "print.h"

static const char* level_name(
    enum log_level level
)
{
    switch (level) {
        case LOG_DEBUG:
            return "DEBUG";

        case LOG_INFO:
            return "INFO";

        case LOG_WARNING:
            return "WARN";

        case LOG_ERROR:
            return "ERROR";

        case LOG_FATAL:
            return "FATAL";

        default:
            return "????";
    }
}

void log_init(void)
{
}

void log_message(
    enum log_level level,
    const char* message
)
{
    print_str("[");
    print_str(level_name(level));
    print_str("] ");
    print_str(message);
    print_char('\n');
}

void log_debug(const char* message)
{
    log_message(LOG_DEBUG, message);
}

void log_info(const char* message)
{
    log_message(LOG_INFO, message);
}

void log_warning(const char* message)
{
    log_message(LOG_WARNING, message);
}

void log_error(const char* message)
{
    log_message(LOG_ERROR, message);
}

void log_fatal(const char* message)
{
    log_message(LOG_FATAL, message);
}
