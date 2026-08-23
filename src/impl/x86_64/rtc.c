#include "rtc.h"
#include "io.h"

static uint8_t cmos_read(
    uint8_t register_number
)
{
    outb(0x70, register_number);
    return inb(0x71);
}

static uint8_t bcd_to_binary(
    uint8_t value
)
{
    return (uint8_t)(
        (value & 0x0F) +
        ((value >> 4) * 10)
    );
}

static void wait_for_update(void)
{
    while (cmos_read(0x0A) & 0x80) {
        __asm__ volatile ("pause");
    }
}

void rtc_read(
    struct rtc_datetime* datetime
)
{
    if (datetime == 0) {
        return;
    }

    uint8_t second;
    uint8_t minute;
    uint8_t hour;
    uint8_t day;
    uint8_t month;
    uint8_t year;
    uint8_t century;

    uint8_t second_check;

    do {
        wait_for_update();

        second = cmos_read(0x00);
        minute = cmos_read(0x02);
        hour = cmos_read(0x04);
        day = cmos_read(0x07);
        month = cmos_read(0x08);
        year = cmos_read(0x09);
        century = cmos_read(0x32);

        second_check = cmos_read(0x00);
    } while (second != second_check);

    uint8_t register_b =
        cmos_read(0x0B);

    if ((register_b & 0x04) == 0) {
        second = bcd_to_binary(second);
        minute = bcd_to_binary(minute);
        hour = bcd_to_binary(hour);
        day = bcd_to_binary(day);
        month = bcd_to_binary(month);
        year = bcd_to_binary(year);

        if (century != 0) {
            century =
                bcd_to_binary(century);
        }
    }

    if ((register_b & 0x02) == 0) {
        uint8_t pm =
            hour & 0x80;

        hour &= 0x7F;

        if (pm && hour < 12) {
            hour += 12;
        }

        if (!pm && hour == 12) {
            hour = 0;
        }
    }

    datetime->second = second;
    datetime->minute = minute;
    datetime->hour = hour;

    datetime->day = day;
    datetime->month = month;

    if (century != 0) {
        datetime->year =
            (uint16_t)century * 100 +
            year;
    } else {
        datetime->year =
            (uint16_t)(2000 + year);
    }
}
