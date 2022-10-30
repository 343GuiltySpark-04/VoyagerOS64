#pragma once

#ifndef _LOCK_C_H
#define _LOCK_C_H

#include "global_defs.h"
#include <stdbool.h>

extern int atomic_lock();

extern int atomic_unlock();

typedef int spinlock_t;

#define SPINLOCK_INIT 0

static inline bool spinlock_test_and_acq(spinlock_t *lock)
{
    return CAS(lock, 0, 1);
}

static inline void spinlock_acquire(spinlock_t *lock)
{
    for (;;)
    {
        if (spinlock_test_and_acq(lock))
        {
            break;
        }
#if defined(__x86_64__)
        asm volatile("pause");
#endif
    }
}

static inline void spinlock_release(spinlock_t *lock)
{
    CAS(lock, 1, 0);
}

#endif