#include "timer.h"
#include "io.h"

static volatile uint64_t ticks = 0;

static uint64_t frequency = 100;

void timer_init(uint32_t requested_frequency)
{
    if (requested_frequency == 0) {
        requested_frequency = 100;
    }

    frequency = requested_frequency;

    uint32_t divisor =
        1193182u / requested_frequency;

    if (divisor == 0) {
        divisor = 1;
    }

    if (divisor > 65535) {
        divisor = 65535;
    }

    outb(0x43, 0x36);

    outb(
        0x40,
        (uint8_t)(divisor & 0xFF)
    );

    outb(
        0x40,
        (uint8_t)((divisor >> 8) & 0xFF)
    );
}

void timer_tick(void)
{
    ticks++;
}

uint64_t timer_ticks(void)
{
    return ticks;
}

uint64_t timer_frequency(void)
{
    return frequency;
}

uint64_t timer_uptime_seconds(void)
{
    if (frequency == 0) {
        return 0;
    }

    return ticks / frequency;
}

void timer_sleep(uint64_t milliseconds)
{
    uint64_t start = ticks;

    uint64_t wait_ticks =
        (milliseconds * frequency + 999) / 1000;

    if (wait_ticks == 0) {
        wait_ticks = 1;
    }

    while ((ticks - start) < wait_ticks) {
        __asm__ volatile ("hlt");
    }
}
