#include <stdint.h>
#include <stddef.h>
#include "include/io.h"
#include "include/sched.h"
#include "include/kernel.h"
#include "include/string.h"
#include "include/lock.h"
#include "include/printf.h"

#define IO_STACK_SIZE 4096 - 1

static spinlock_t streamlock_t = SPINLOCK_INIT;

/**
 * @brief Push an I / O pair onto the IO stack. This is used to signal that we are waiting for data to arrive on the process that started the pair.
 * @param a
 * @param s
 * @param pid The id of the process that started the pair being operated on.
 * @param data The data to be passed to the process on the IO stack
 */
void push_io(struct active_tube *a, struct standby_tube *s, uint64_t pid, ...)
{

    va_list data;
    va_start(data, pid);

    spinlock_test_and_acq(&streamlock_t);

    struct tube_process *p;

    // Returns the process object for the active process.
    if (active_pid == pid)
    {

        p = &a->processes[0];
    }
    else
    {

        // Find the active process in the process list.
        for (uint64_t i = a->process_count - 1; i > 0; i--)
        {

            // Find the active process.
            if (a->processes[i].id == pid)
            {

                p = &a->processes[i];
                break;
            }
        }

        // Find the active process in the process list.
        for (uint64_t i = s->process_count - 1; i > 0; i--)
        {

            // Find the active process.
            if (s->processes[i].id == pid)
            {

                p = &s->processes[i];
                break;
            }
        }
    }

    // Pushes data to the IO stack.
    if (p->io_stack_top == IO_STACK_SIZE)
    {

        kerror_mode = 1;

        printf_("%s\n", "ERROR: Processes IO stack overflow!");

        kerror_mode = 0;
    }
    else
    {

        p->io_stack_top++;
        p->io_stack[p->io_stack_top] = data;
    }

    va_end(data);

    spinlock_release(&streamlock_t);
}

/**
 * @brief Pop IO stack. This is used to clean up the I / O stack when we exit a non - daemon process.
 * @param a
 * @param s
 * @param pid The ID of the process that exited. If this is the exec_pid we're popping the IO stack from the top of the stack.
 * @return 0 on success 1 on failure. In this case an error message is printed to stderr and kerror_mode is set to
 */
char pop_io(struct active_tube *a, struct standby_tube *s, uint64_t pid)
{

    // spinlock_acquire(&streamlock_t);

    struct tube_process *p;

    // Returns the process object for the active process.
    if (pid == exec_pid)
    {

        p = &a->processes[0];
    }
    else
    {

        // Find the active process in the process list.
        for (uint64_t i = a->process_count - 1; i > 0; i--)
        {

            // Find the active process.
            if (a->processes[i].id == pid)
            {

                p = &a->processes[i];
                break;
            }
        }

        // Find the active process in the process list.
        for (uint64_t i = s->process_count - 1; i > 0; i--)
        {

            // Find the active process.
            if (s->processes[i].id == pid)
            {

                p = &s->processes[i];
                break;
            }
        }
    }

    // Processes IO stack underflow. If the IO stack underflow is not available in the stack it will be ignored.
    if (p->io_stack_top == -1)
    {

        kerror_mode = 1;

        printf_("%s\n", "ERROR: Processes IO stack underflow!");

        kerror_mode = 0;
    }
    else
    {

        p->io_stack_top--;
    }

    // spinlock_release(&streamlock_t);

    return p->io_stack[p->io_stack_top + 1];
}

// only single char support for now keeping it simple during inital testing

/**
 * @brief This is the function that is called when stdin is ready. It pushes data onto the stdio stack
 * @param flags The flags that are passed to stdin
 * @param data The data to be pushed onto the stdio
 */
void stdin(char flags, ...)
{

    va_list data;
    va_start(data, flags);

    // push_io active_tube active_tube active_pid data i 256
    if (flags == "" || "K" || "k")
    {

        // Pushes the active tube to the active tube.
        for (int i = 0; i < 256; i++)
        {

            push_io(&k_active_tube, &k_standby_tube, active_pid, data[i]);
        }
    }

    va_end(data);

    return;
}

/**
 * @brief Print information about the active tube. This is a debugging function to be used in conjunction with the -- print option
 * @param flags " p " for print information about the active tube
 * @param a pointer to the active tube
 * @param s pointer to the standby tube ( - 1
 */
void stdout(char flags, struct active_tube *a, struct standby_tube *s)
{

    // Prints out the active process.
    if (flags == "" || "p" || "P")
    {
        struct tube_process *p;

        // Find the active process.
        if (active_pid == active_pid)
        {

            p = &a->processes[0];
        }
        else
        {

            // Find the active process in the process list.
            for (uint64_t i = a->process_count - 1; i > 0; i--)
            {

                // Find the active process.
                if (a->processes[i].id == active_pid)
                {

                    p = &a->processes[i];
                    break;
                }
            }

            // Find the active process in the process list.
            for (uint64_t i = s->process_count - 1; i > 0; i--)
            {

                // Find the active process.
                if (s->processes[i].id == active_pid)
                {

                    p = &s->processes[i];
                    break;
                }
            }
        }

        printf_("%s\n", p->io_stack);
    }

    return;
}
