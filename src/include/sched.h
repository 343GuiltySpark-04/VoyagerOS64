#pragma once

#ifndef _SCHED_H
#define _SCHED_H

#include <stdint.h>
#include <stdbool.h>
#include "global_defs.h"

#define SYSCALL_YIELD 1
#define SYSCALL_FORK 2
#define SYSCALL_EXIT 3

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

struct PACKED tube_process
{

    uint64_t id;
    bool high_priority;
    uint64_t allocated_time;
    uint8_t syscall;
    char name[256];
    bool repeat;
    bool immortal;
    uint8_t exit_code;
    uint64_t child_ids[256];
    uint64_t parent_id; // NULL if N/A
};

struct PACKED standby_tube
{

    struct tube_process *processes;

    uint64_t process_count;

    uint64_t current_standby;
};

struct PACKED active_tube
{

    struct tube_process *processes;

    uint64_t process_count;

    uint64_t current_active;

    uint8_t prev_exit_code;
};

struct Process create_process(uint64_t id, uint64_t priority, bool mortality, char name[256]);
struct tube_process create_tube_process(bool high_priority, bool immortal, bool repeat, char name[256]);
void shift_standby(struct standby_tube *standby_tube, struct active_tube *active_tube);
void remove_active(struct active_tube *tube);
void add_tube_process(struct standby_tube *tube, struct tube_process p);
void shift_active(struct standby_tube *standby_tube, struct active_tube *active_tube);
void add_process(struct Scheduler *scheduler, struct Process p);
void schedule(struct Scheduler *scheduler, uint64_t time_quantum);
void tube_schedule(struct standby_tube *standby, struct active_tube *active, uint64_t quantum);
void fork(struct Scheduler *scheduler);
void exit(struct Scheduler *scheduler);
uint64_t generate_id();
uint64_t pid_hash(char name[256]);
void init_sched(struct standby_tube *s_tube, struct active_tube *a_tube);

#endif
