#pragma once

#ifndef _LOCK_C_H
#define _LOCK_C_H

#include "global_defs.h"
#include <stdbool.h>

extern int atomic_lock();

extern int atomic_unlock();

typedef int spinlock_t;

#define SPINLOCK_INIT 0

struct smartlock
{
    size_t refcount;
    struct thread *thread;
};

#define SMARTLOCK_INIT \
    {                  \
        0, NULL        \
    }

void smartlock_acquire(struct smartlock *smartlock);
void smartlock_release(struct smartlock *smartlock);

/**
* @brief Test and acquirer a spinlock. This is equivalent to spinlock_test () followed by spinlock_acq ().
* @param lock
* @return true if the lock was acquired false otherwise. Note that this does not guarantee that the lock has been acquired
*/
static inline bool spinlock_test_and_acq(spinlock_t *lock)
{
    return CAS(lock, 0, 1);
}

/**
* @brief Acquire a spin lock. This is a blocking version of spinlock_test_and_acq ()
* @param lock
*/
static inline void spinlock_acquire(spinlock_t *lock)
{
    // Pauses the spinlock test and acquisition.
    for (;;)
    {
        // If spinlock_test_and_acq is true then the lock is not locked.
        if (spinlock_test_and_acq(lock))
        {
            break;
        }
#if defined(__x86_64__)
        asm volatile("pause");
#endif
    }
}

/**
* @brief Release a spinlock. This is equivalent to a CAS followed by a release. The lock is released in the process of acquiring it so if you are holding it you must call spinlock_release () before you can use it.
* @param lock
*/
static inline void spinlock_release(spinlock_t *lock)
{
    CAS(lock, 1, 0);
}

#endif