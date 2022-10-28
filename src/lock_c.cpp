#include "include/lock.hpp"

AtomicLock c_lock;

extern "C" int atomic_lock()
{

    c_lock.Lock();

    return 0;
}

extern "C" int atomic_unlock()
{

    c_lock.Unlock();

    return 0;
}
