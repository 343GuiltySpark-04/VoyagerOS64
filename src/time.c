#include "include/io.h"
#include "include/printf.h"
#include "include/pic.h"
#include "include/sched.h"
#include "include/kernel.h"
#include "include/time.h"
#include "include/lock.h"
#include "include/liballoc.h"
#include "include/limine.h"
#include "include/lib/vector.h"
#include "include/memUtils.h"
#include "include/idt.h"
#include "include/apic/lapic.h"
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

struct timespec time_mono = {0, 0};
struct timespec time_real = {0, 0};

static spinlock_t timers_lock = SPINLOCK_INIT;
static VECTOR_TYPE(struct timer *) armed_timers = VECTOR_INIT;

static volatile struct limine_boot_time_request boot_time_req = {

    .id = LIMINE_BOOT_TIME_REQUEST,
    .revision = 0

};

/**
 * @brief Create a new timer.
 * @param when The time at which the timer will expire.
 * @return The new timer or NULL on error
 */
struct timer *timer_new(struct timespec when)
{

    struct timer *timer = ALLOC(struct timer);

    if (timer == NULL)
    {

        return NULL;
    }

    timer->when = when;
    timer_arm(timer);
    //  return timer;
}

/**
 * @brief Arm a timer so it will be fired.
 * @param * timer
 */
void timer_arm(struct timer *timer)
{

    spinlock_test_and_acq(&timers_lock);

    timer->index = armed_timers.length;
    timer->fired = false;

    VECTOR_PUSH_BACK(&armed_timers, timer);
    spinlock_release(&timers_lock);
}

/**
 * @brief Remove a timer from the armed timers list
 * @param * timer
 * @return 0 on success - ENOMEM on OOM
 */
void timer_disarm(struct timer *timer)
{

    spinlock_test_and_acq(&timers_lock);

    if (armed_timers.length == 0 || timer->index == -1 || (size_t)timer->index >= armed_timers.length)
    {
        goto cleanup;
    }

    armed_timers.data[timer->index] = VECTOR_ITEM(&armed_timers, armed_timers.length - 1);
    VECTOR_ITEM(&armed_timers, timer->index)->index = timer->index;
    VECTOR_REMOVE(&armed_timers, armed_timers.length - 1);

    timer->index = -1;

cleanup:
    spinlock_release(&timers_lock);
}

/**
 * @brief This function is called by limine_init () to initialize the time.
 * @return None. Side effects : None
 */
void time_init(void)
{

    struct limine_boot_time_response *boot_time_resp = boot_time_req.response;

    time_real.tv_sec = boot_time_resp->boot_time;

    init_PIT();
}

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

/**
 * @brief Get the PIT count.
 * @return uint16_t The number of pit
 */
uint16_t pit_get_current_count(void)
{
    outb(0x43, 0x00);
    uint8_t lo = inb(0x40);
    uint8_t hi = inb(0x40) << 8;
    return ((uint16_t)hi << 8) | lo;
}

/**
 * @brief Set PIT reload value.
 * @param new_count Value to set in 16 bits
 */
void pit_set_reload_value(uint16_t new_count)
{
    outb(0x43, 0x34);
    outb(0x40, (uint8_t)new_count);
    outb(0x40, (uint8_t)(new_count >> 8));
}

/**
 * @brief Set PIT frequency in Hz
 * @param frequency Frequency in Hz to
 */
void pit_set_frequency(uint64_t frequency)
{
    uint64_t new_divisor = PIT_DIVIDEND / frequency;
    if (PIT_DIVIDEND % frequency > frequency / 2)
    {
        new_divisor++;
    }
    pit_set_reload_value((uint16_t)new_divisor);
}

/**
 * @brief This is the clock handler for the system
 */
void sys_clock_handler()
{

    struct timespec interval = {

        .tv_sec = 0,
        .tv_nsec = 1000000000 / PIT_FREQ

    };

    time_mono = timespec_add(time_mono, interval);
    time_real = timespec_add(time_real, interval);

    if (spinlock_test_and_acq(&timers_lock))
    {
        for (size_t i = 0; i < armed_timers.length; i++)
        {
            struct timer *timer = VECTOR_ITEM(&armed_timers, i);
            if (timer->fired)
            {
                continue;
            }

            timer->when = timespec_sub(timer->when, interval);
            if (timer->when.tv_sec == 0 && timer->when.tv_nsec == 0)
            {
                // event_trigger(&timer->event, false);
                timer->fired = true;
            }
        }

        spinlock_release(&timers_lock);
    }

    if (timer_fired == false)
    {

        timer_fired = true;
    }

    system_timer_fractions++;
    system_timer_ms++;
}

/**
 * @brief Sys clock handler alternative. This is called by the interrupt handler to indicate that the system clock has expired
 */
void sys_clock_handler_alt()
{

    if (timer_fired == false)
    {

        timer_fired = true;
    }

    system_timer_fractions++;
    system_timer_ms++;
}

uint8_t task_timer_count = 0;

/**
 * @brief Handler for task switch.
 * @return Nothing. Side Effects : None
 */
void task_switch_handler()
{

    printf_("%s\n", "Switched!");

    task_timer_count = 0;
}

/**
 * @brief This function is used to convert a 64 - bit value into a vector.
 * @param delta The 64 - bit value to convert.
 * @param vector The vector to convert
 */
void delta_int(uint64_t delta, uint8_t vector)
{
}

/**
 * @brief Initialize PIT to a known
 */
void init_PIT()
{

    pit_set_frequency(PIT_FREQ);

    pic_unmask_irq(0);

    printf_("%s\n", "PIT Online!");
}

/**
 * @brief Get update flag from CMOS
 * @return 1 if update flag is
 */
int get_update_flag()
{

    outb(CMOS_ADDR, 0x0A);
    return (inb(CMOS_DATA) & 0x80);
}

/**
 * @brief Read a byte from a CMOS RTC register
 * @param reg register to read from ( 0 - 15 )
 * @return byte read from the register
 */
uint8_t get_RTC_register(int reg)
{

    outb(CMOS_ADDR, reg);
    return inb(CMOS_DATA);
}

/**
 * @brief Read the RTC registers
 */
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

/**
 * @brief Print the date of the current RTC.
 * @return N / A Returns 0
 */
void print_date()
{

    // TODO: Work out the kinks

    read_rtc();

    printf_("%s", weekday[dayofweek(year, month, day)]);

    printf_("%s", " ");

    printf_("%i", day);

    printf_("%s", " ");

    printf_("%s", month_str[month - 1]);

    printf_("%s", " ");

    printf_("%i", year);

    printf_("%s\n", " (VERY WIP)");
}

// Courtesy of Tomohiko Sakamoto
/* 1 <= m <= 12,  y > 1752 (in the U.K.) */

// Determane day of the week from the month year and date.
int dayofweek(y, m, d)
{
    static int t[] = {0, 3, 2, 5, 0, 3, 5, 1, 4, 6, 2, 4};
    if (m < 3)
    {
        y -= 1;
    }
    return (y + y / 4 - y / 100 + y / 400 + t[m - 1] + d) % 7;
}

/**
 * @brief Prints the system time.
 * @return N / A Returns 0
 */
void print_sys_time()
{

    printf_("%s", "Elapsed system time is: ");
    printf_("%i", system_timer_ms);
    printf_("%s", ".");
    printf_("%i", system_timer_fractions);
    printf_("%s\n", " ms.");
}

/**
 * @brief Print load time to stdout.
 * @return void. Side effects : None
 */
void print_load_time()
{

    printf_("%s\n", "Kernel Loaded");

    printf_("%s", "Loadtime roughly: ");

    printf_("%i", system_timer_ms);

    printf_("%s", ".");

    printf_("%i", system_timer_fractions);

    printf_("%s\n", " ms.");
}