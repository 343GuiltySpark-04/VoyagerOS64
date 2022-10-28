#pragma once

#ifndef _TIME_H
#define _TIME_H

extern uint64_t system_timer_ms;

extern uint64_t system_timer_fractions;

extern uint8_t second, minute, hour, day, month, year;

void sys_clock_handler();

void init_PIT();

void read_rtc();

void print_date();

void print_sys_time();

#endif