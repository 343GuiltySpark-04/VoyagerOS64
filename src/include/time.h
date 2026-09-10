#pragma once
#ifndef _TIME_H
#define _TIME_H

#include "global_defs.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define PIT_FREQ 750
#define PIT_DIVIDEND 1193180

typedef long time_t;

struct timespec
{
    time_t tv_sec;
    long   tv_nsec;
};

struct timer
{
    ssize_t         index;
    bool            fired;
    struct timespec when;
};

extern struct timespec time_mono;
extern struct timespec time_real;

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

extern uint64_t system_timer_ms;
extern uint64_t system_timer_fractions;

extern uint8_t  second, minute, hour, day, month;
extern uint16_t year;

void sys_clock_handler(void);
void init_PIT(void);
void read_rtc(void);
void print_date(void);
void print_sys_time(void);
int  dayofweek(int y, int m, int d);

struct timer *timer_new(struct timespec when);
void          timer_arm(struct timer *timer);
void          timer_disarm(struct timer *timer);
void          print_load_time(void);

uint16_t pit_get_current_count(void);
void     pit_set_reload_value(uint16_t new_count);
void     pit_set_frequency(uint64_t frequency);
void     sys_clock_handler_alt(void);
void     time_init(void);
void     pit_sleep(uint64_t ms);

#endif
