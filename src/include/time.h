#pragma once

#ifndef _TIME_H
#define _TIME_H

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>
#include "global_defs.h"

#define PIT_FREQ 1000

typedef long time_t;

struct timespec
{
    time_t tv_sec;
    long tv_nsec;
};

struct timer
{
    ssize_t index;
    bool fired;
    struct timespec when;
    // struct event event;
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
        {
            a.tv_sec--;
        }
        else
        {
            a.tv_sec = a.tv_nsec = 0;
        }
    }
    else
    {
        a.tv_nsec -= b.tv_nsec;
    }

    if (b.tv_sec > a.tv_sec)
    {
        a.tv_sec = a.tv_nsec = 0;
    }
    else
    {
        a.tv_sec -= b.tv_sec;
    }

    return a;
}

extern uint64_t system_timer_ms;

extern uint64_t system_timer_fractions;

extern uint8_t second, minute, hour, day, month, year;

void sys_clock_handler();

void init_PIT();

void read_rtc();

void print_date();

void print_sys_time();

struct timer *timer_new(struct timespec when);
void timer_arm(struct timer *timer);
void timer_disarm(struct timer *timer);
void print_load_time();

void time_init(void);

#endif