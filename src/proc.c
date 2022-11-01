#include <stdint.h>
#include <stddef.h>
#include "include/liballoc.h"
#include "include/lib/hashmap.h"
#include "include/printf.h"
#include "include/paging/frameallocator.h"
#include "include/paging/paging.h"
#include "include/proc.h"

static struct smartlock futex_lock = SMARTLOCK_INIT;
static HASHMAP_TYPE(struct event *) futex_hashmap = HASHMAP_INIT(256);

void proc_init(void)
{
    uintptr_t phys = 0;
    HASHMAP_INSERT(&futex_hashmap, &phys, sizeof(phys), NULL);
}

int syscall_futex_wait(void *_, int *ptr, int expected)
{
    (void)_;



    int ret = -1;

    struct thread *thread = sched_current_thread();
    struct process *proc = thread->process;

    if (*ptr != expected)
    {
        // errno = EAGAIN;
        goto cleanup;
    }

    uintptr_t phys = TranslateToPhysicalMemoryAddress(proc->pagemap);

    smartlock_acquire(&futex_lock);

    smartlock_release(&futex_lock);

cleanup:

    return ret;
}

int syscall_futex_wake(void *_, int *ptr)
{
    (void)_;

   

    struct thread *thread = sched_current_thread();
    struct process *proc = thread->process;

    // Make sure the page isn't demand paged
    *(volatile int *)ptr;

    uintptr_t phys = TranslateToPhysicalMemoryAddress(proc->pagemap);

    smartlock_acquire(&futex_lock);

    struct event *event = NULL;
    if (!HASHMAP_GET(&futex_hashmap, event, &phys, sizeof(phys)))
    {
        goto cleanup;
    }

   // event_trigger(event, true);

cleanup:
    smartlock_release(&futex_lock);

    return 0;
}

mode_t syscall_umask(void *_, mode_t mask)
{
    (void)_;

   

    struct thread *thread = sched_current_thread();
    struct process *proc = thread->process;

    mode_t old_mask = proc->umask;
    proc->umask = mask;

   
    return old_mask;
}
