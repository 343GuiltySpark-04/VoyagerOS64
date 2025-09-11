#include "include/lock.hpp"

AtomicLock c_lock;

/**
* @brief Locks the mutex. This is a wrapper around c_lock. Lock ()
* @return 0 on success non - zero
*/
extern "C" int atomic_lock()
{

    c_lock.Lock();

    return 0;
}

/**
* @brief Unlock the mutex. This is an atomic operation.
* @return 0 on success non - zero
*/
extern "C" int atomic_unlock()
{

    c_lock.Unlock();

    return 0;
}
