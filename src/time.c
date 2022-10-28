#include "include/io.h"
#include "include/printf.h"
#include "include/pic.h"
#include <stdint.h>

#define CHANNEL_ZERO 0x40
#define MODE_CMD 0x43
#define CURRENT_YEAR 2022
#define CMOS_ADDR 0x70
#define CMOS_DATA 0x71

extern void config_PIT(uint8_t freq);

int8_t century_register = 0x00;

uint8_t second, minute, hour, day, month, year;

uint64_t system_timer_ms = 0;

uint64_t system_timer_fractions = 0;

static const char *weekday[] = {

    "Sun",
    "Mon",
    "Tue",
    "Wen",
    "Thu",
    "Fri",
    "Sat"

};

static const char *month_str[] = {

    "Jan",
    "Feb",
    "Mar",
    "Apr",
    "May",
    "Jun",
    "Jul",
    "Aug",
    "Sep",
    "Oct",
    "Nov",
    "Dec"

};

void sys_clock_handler()
{

    system_timer_fractions++;
    system_timer_ms++;
}

void init_PIT()
{

    config_PIT(1000);

    pic_unmask_irq(0);

    printf_("%s\n", "PIT Online!");
}

int get_update_flag()
{

    outb(CMOS_ADDR, 0x0A);
    return (inb(CMOS_DATA) & 0x80);
}

uint8_t get_RTC_register(int reg)
{

    outb(CMOS_ADDR, reg);
    return inb(CMOS_DATA);
}

void read_rtc()
{
    unsigned char century;
    unsigned char last_second;
    unsigned char last_minute;
    unsigned char last_hour;
    unsigned char last_day;
    unsigned char last_month;
    unsigned char last_year;
    unsigned char last_century;
    unsigned char registerB;

    // Note: This uses the "read registers until you get the same values twice in a row" technique
    //       to avoid getting dodgy/inconsistent values due to RTC updates

    while (get_update_flag())
        ; // Make sure an update isn't in progress
    second = get_RTC_register(0x00);
    minute = get_RTC_register(0x02);
    hour = get_RTC_register(0x04);
    day = get_RTC_register(0x07);
    month = get_RTC_register(0x08);
    year = get_RTC_register(0x09);
    if (century_register != 0)
    {
        century = get_RTC_register(century_register);
    }

    do
    {
        last_second = second;
        last_minute = minute;
        last_hour = hour;
        last_day = day;
        last_month = month;
        last_year = year;
        last_century = century;

        while (get_update_flag())
            ; // Make sure an update isn't in progress
        second = get_RTC_register(0x00);
        minute = get_RTC_register(0x02);
        hour = get_RTC_register(0x04);
        day = get_RTC_register(0x07);
        month = get_RTC_register(0x08);
        year = get_RTC_register(0x09);
        if (century_register != 0)
        {
            century = get_RTC_register(century_register);
        }
    } while ((last_second != second) || (last_minute != minute) || (last_hour != hour) ||
             (last_day != day) || (last_month != month) || (last_year != year) ||
             (last_century != century));

    registerB = get_RTC_register(0x0B);

    // Convert BCD to binary values if necessary

    if (!(registerB & 0x04))
    {
        second = (second & 0x0F) + ((second / 16) * 10);
        minute = (minute & 0x0F) + ((minute / 16) * 10);
        hour = ((hour & 0x0F) + (((hour & 0x70) / 16) * 10)) | (hour & 0x80);
        day = (day & 0x0F) + ((day / 16) * 10);
        month = (month & 0x0F) + ((month / 16) * 10);
        year = (year & 0x0F) + ((year / 16) * 10);
        if (century_register != 0)
        {
            century = (century & 0x0F) + ((century / 16) * 10);
        }
    }

    // Convert 12 hour clock to 24 hour clock if necessary

    if (!(registerB & 0x02) && (hour & 0x80))
    {
        hour = ((hour & 0x7F) + 12) % 24;
    }

    // Calculate the full (4-digit) year

    if (century_register != 0)
    {
        year += century * 100;
    }
    else
    {
        year += (CURRENT_YEAR / 100) * 100;
        if (year < CURRENT_YEAR)
            year += 100;
    }
}

void print_date()
{

    // TODO: Work out the kinks

    read_rtc();

    printf_("%s", weekday[dayofweek(year, month, day)]);

    printf_("%s", " ");

    printf_("%i", day);

    printf_("%s", " ");

    printf_("%s", month_str[month]);

    printf_("%s", " ");

    printf_("%i", year);

    printf_("%s\n", " (VERY WIP)");
}

// Courtesy of Tomohiko Sakamoto

dayofweek(y, m, d) /* 1 <= m <= 12,  y > 1752 (in the U.K.) */
{
    static int t[] = {0, 3, 2, 5, 0, 3, 5, 1, 4, 6, 2, 4};
    if (m < 3)
    {
        y -= 1;
    }
    return (y + y / 4 - y / 100 + y / 400 + t[m - 1] + d) % 7;
}
