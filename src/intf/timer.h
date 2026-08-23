#pragma once

#include <stdint.h>

void timer_init(uint32_t frequency);

void timer_tick(void);

uint64_t timer_ticks(void);
uint64_t timer_frequency(void);

uint64_t timer_uptime_seconds(void);

void timer_sleep(uint64_t milliseconds);
