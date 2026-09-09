#pragma once

#ifndef _SCHED_H
#define _SCHED_H

#include "global_defs.h"
#include <stdbool.h>
#include <stdint.h>

#define SYSCALL_YIELD 1
#define SYSCALL_FORK 2
#define SYSCALL_EXIT 3

typedef enum
{
    PROC_READY,
    PROC_RUNNING,
    PROC_BLOCKED

} proc_state_t;

typedef struct process
{
    int pid;

    uint64_t *rsp;

    proc_state_t state;

    struct process *next;
} process_t;

extern bool allow_sched;

extern process_t *current;

void switch_to(process_t *next);

process_t *create_process(void (*entry)(void));
void       schedule(void);
void       init_scheduler(void);
void       scheduler_start(void);

#endif
