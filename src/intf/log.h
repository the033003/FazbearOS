#pragma once

enum log_level {
    LOG_DEBUG = 0,
    LOG_INFO,
    LOG_WARNING,
    LOG_ERROR,
    LOG_FATAL
};

void log_init(void);

void log_message(
    enum log_level level,
    const char* message
);

void log_debug(const char* message);
void log_info(const char* message);
void log_warning(const char* message);
void log_error(const char* message);
void log_fatal(const char* message);
