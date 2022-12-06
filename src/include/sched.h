#pragma once

#ifndef _SCHED_H
#define _SCHED_H

#include <stdint.h>
#include <stdbool.h>

extern uint64_t next_id;
extern bool timer_fired;
extern const uint64_t quantum;

// Data structure to represent a process
struct Process
{
    int id;
    int priority;
    int allocated_time;
};

// Data structure to represent the round-robin scheduler
struct Scheduler
{
    // Array to store processes
    struct Process *processes;

    // Number of processes
    int n;
};

struct Process create_process(int id, int priority);
void add_process(struct Scheduler *scheduler, struct Process p);
void schedule(struct Scheduler *scheduler, int time_quantum);

#endif
