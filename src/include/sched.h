#pragma once

#ifndef _SCHED_H
#define _SCHED_H

#include <stdint.h>
#include <stddef.h>
#include "global_defs.h"
#define TASK_RUNNING 1

typedef struct PACKED TCB
{

    uint64_t task_top;
    uint64_t task_addr;
    uint64_t next_addr;
    uint8_t task_state;

} tcb_t;

extern void switch_to_task(TCB *next_thread);

#endif