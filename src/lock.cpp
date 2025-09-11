#include "include/lock.hpp"

AtomicLock::AtomicLock() : locked(false){};

/**
* @brief Check if lock is locked.
* @return true if lock is locked
*/
bool AtomicLock::IsLocked() const
{
    uint32_t result = 0;

    __atomic_load(&locked, &result, __ATOMIC_SEQ_CST);

    return result;
}

/**
* @brief Locks the mutex. This is a non - blocking operation.
* @return true if the mutex was
*/
void AtomicLock::Lock()
{
    uint32_t expected = false;
    uint32_t desired = true;

    while (!__atomic_compare_exchange(&locked, &expected, &desired, false, __ATOMIC_ACQUIRE, __ATOMIC_RELAXED))
    {
        expected = false;

        asm volatile("pause");
    }
}

/**
* @brief \ brief Force the lock to be unlocked.
* @return true if the lock was
*/
void AtomicLock::ForceLock()
{
    locked = true;
}

/**
* @brief Unlock the lock. This is equivalent to releasing the lock but does not block
*/
void AtomicLock::Unlock()
{
    __atomic_store_n(&locked, false, __ATOMIC_RELEASE);

    locked = false;
}

ScopedLock::ScopedLock(AtomicLock &value) : lock(value)
{
    lock.Lock();
}

ScopedLock::~ScopedLock()
{
    lock.Unlock();
}
