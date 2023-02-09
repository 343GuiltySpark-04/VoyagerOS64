#pragma once

#ifndef _SCHED_H
#define _SCHED_H

#include <stdint.h>
#include <stdbool.h>

extern uint64_t next_id;
extern bool timer_fired;
extern const uint64_t quantum;
extern bool sched_started;
extern high_yield_int();

// Data structure to represent a process
struct Process
{
    uint64_t id;
    uint64_t priority;
    uint64_t allocated_time;
    uint8_t syscall;
    char name[256];
    bool immortal;
};
// Data structure to represent the round-robin scheduler
struct Scheduler
{
    // Array to store processes
    struct Process *processes;
    // Number of processes
    uint64_t n;

    uint64_t current_process;
};

struct tube_process
{

    uint64_t id;
    bool high_priority;
    uint64_t allocated_time;
    uint8_t syscall;
    char name[256];
    bool repeat;
    bool immortal;
};

struct standby_tube
{

    struct tube_process *processes;

    uint32_t process_count;
};

struct active_tube
{

    struct tube_process *processes;

    uint32_t process_count;
};

struct Process create_process(uint64_t id, uint64_t priority, bool mortality, char name[256]);
struct tube_process create_tube_process(bool high_priority, bool immortal, bool repeat, char name[256]);
void add_process(struct Scheduler *scheduler, struct Process p);
void schedule(struct Scheduler *scheduler, uint64_t time_quantum);
void fork(struct Scheduler *scheduler);
void exit(struct Scheduler *scheduler);
uint64_t generate_id();
uint64_t pid_hash(char name[256]);
void init_sched();

#endif
