#include <stddef.h>
#include "include/lock.h"
#include "include/proc.h"
#include "include/kernel.h"
#include "include/printf.h"

void smartlock_acquire(struct smartlock *smartlock)
{
    struct thread *thread = sched_current_thread();
    if (smartlock->thread == thread && smartlock->refcount > 0)
    {
        goto end;
    }
    while (!CAS(&smartlock->thread, NULL, thread))
    {
        asm("pause");
    }
end:
    smartlock->refcount++;
}

void smartlock_release(struct smartlock *smartlock)
{
    struct thread *thread = sched_current_thread();
    if (smartlock->thread != thread)
    {
        kerror_mode++;
        printf_("%s\n", "Invalid smartlock release");
        kerror_mode--;
    }
    if (smartlock->refcount == 0)
    {
        kerror_mode++;
        printf_("%s\n", "Smartlock release refcount is 0");
        kerror_mode--;
    }
    smartlock->refcount--;
    if (smartlock->refcount == 0)
    {
        smartlock->thread = NULL;
    }
}