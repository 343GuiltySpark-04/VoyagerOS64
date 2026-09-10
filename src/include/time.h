/**
 * @file time.h
 * @brief RTC, PIT, and kernel timekeeping interfaces.
 * @ingroup time
 */
#pragma once
#ifndef _TIME_H
#define _TIME_H

#include "global_defs.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/** PIT programming frequency currently requested by VoyagerOS64. */
#define PIT_FREQ 750
/** Input clock used by the legacy 8253/8254 PIT. */
#define PIT_DIVIDEND 1193180

typedef long time_t;

/** @brief Seconds/nanoseconds time representation used by kernel timers. */
struct timespec
{
    time_t tv_sec;
    long   tv_nsec;
};

/** @brief Small software-timer record used by the current timer layer. */
struct timer
{
    ssize_t         index;
    bool            fired;
    struct timespec when;
};

/** Monotonic time accumulator maintained by the current timer subsystem. */
extern struct timespec time_mono;
/** Real-time accumulator/state maintained by the current timer subsystem. */
extern struct timespec time_real;

/**
 * @brief Add two timespec values with nanosecond carry.
 * @param a First value.
 * @param b Second value.
 * @return Sum of @p a and @p b.
 */
static inline struct timespec timespec_add(struct timespec a, struct timespec b)
{
    if (a.tv_nsec + b.tv_nsec > 999999999)
    {
        a.tv_nsec = (a.tv_nsec + b.tv_nsec) - 1000000000;
        a.tv_sec++;
    }
    else
    {
        a.tv_nsec += b.tv_nsec;
    }
    a.tv_sec += b.tv_sec;
    return a;
}

/**
 * @brief Subtract one timespec from another using the current kernel helper.
 * @param a Minuend.
 * @param b Subtrahend.
 * @return Non-negative result according to the helper's current saturation/
 *         borrow behavior.
 *
 * @note The timer subsystem is still undergoing cleanup after 0.0.5 memory and
 * scheduler stabilization. Do not treat this helper as a fully qualified POSIX
 * timespec implementation yet.
 */
static inline struct timespec timespec_sub(struct timespec a, struct timespec b)
{
    if (b.tv_nsec > a.tv_nsec)
    {
        a.tv_nsec = 999999999 + (b.tv_nsec - a.tv_nsec);
        if (a.tv_sec > 0)
            a.tv_sec--;
        else
            a.tv_sec = a.tv_nsec = 0;
    }
    else
    {
        a.tv_nsec -= b.tv_nsec;
    }

    if (b.tv_sec > a.tv_sec)
        a.tv_sec = a.tv_nsec = 0;
    else
        a.tv_sec -= b.tv_sec;

    return a;
}

/** Historical millisecond-named timer counter. */
extern uint64_t system_timer_ms;
/** Fractional/auxiliary timer accumulator used by the current implementation.
 */
extern uint64_t system_timer_fractions;

/** RTC calendar fields populated by read_rtc(). */
extern uint8_t second, minute, hour, day, month;
/** Full RTC year value after century handling. */
extern uint16_t year;

/** @brief Primary PIT/system-clock interrupt handler. */
void sys_clock_handler(void);
/** @brief Program the PIT using the current default timer configuration. */
void init_PIT(void);
/** @brief Read a stable calendar/time snapshot from the RTC/CMOS path. */
void read_rtc(void);
/** @brief Print the current calendar date. */
void print_date(void);
/** @brief Print current system time information for VSH diagnostics. */
void print_sys_time(void);

/**
 * @brief Compute weekday index for a Gregorian calendar date.
 * @param y Full year.
 * @param m Month number.
 * @param d Day of month.
 * @return Weekday index used by the date-printing code.
 */
int dayofweek(int y, int m, int d);

/** @brief Allocate a software timer record for a target time. */
struct timer *timer_new(struct timespec when);
/** @brief Arm a software timer. */
void timer_arm(struct timer *timer);
/** @brief Disarm a software timer. */
void timer_disarm(struct timer *timer);
/** @brief Print boot/load-time diagnostics. */
void print_load_time(void);

/** @brief Read the PIT's current counter value. */
uint16_t pit_get_current_count(void);
/** @brief Program the PIT reload/divisor value directly. */
void pit_set_reload_value(uint16_t new_count);
/** @brief Program the PIT for an approximate requested frequency. */
void pit_set_frequency(uint64_t frequency);
/** @brief Alternate system-clock handler retained by the current timer code. */
void sys_clock_handler_alt(void);
/** @brief Initialize RTC/PIT-facing timekeeping state during kernel bring-up.
 */
void time_init(void);
/** @brief Busy/blocking PIT-based sleep helper used by current kernel code. */
void pit_sleep(uint64_t ms);

/**
 * @warning The 0.0.5 release qualification concentrated on memory and the
 * cooperative scheduler. PIT-derived accounting still contains historical
 * millisecond-oriented naming while the PIT is configured at PIT_FREQ=750 Hz;
 * precision/semantic cleanup belongs to later timekeeping work.
 */

#endif
