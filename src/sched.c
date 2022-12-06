#include <stdint.h>
#include "include/printf.h"
#include "include/liballoc.h"
#include "include/sched.h"
#include "include/lock.h"
#include "include/gdt.h"
#include "include/pic.h"
#include <stdbool.h>

#define SAVE_STATE()                       \
    asm volatile("pushq %rax");            \
    asm volatile("pushq %rcx");            \
    asm volatile("pushq %rdx");            \
    asm volatile("pushq %rbx");            \
    asm volatile("pushq %rsp");            \
    asm volatile("pushq %rbp");            \
    asm volatile("pushq %rsi");            \
    asm volatile("pushq %rdi");            \
    asm volatile("pushq %r8");             \
    asm volatile("pushq %r9");             \
    asm volatile("pushq %r10");            \
    asm volatile("pushq %r11");            \
    asm volatile("pushq %r12");            \
    asm volatile("pushq %r13");            \
    asm volatile("pushq %r14");            \
    asm volatile("pushq %r15");            \
    asm volatile("movq %cr3, %rax");       \
    asm volatile("movq %rax, %cr3");       \
    asm volatile("movq %cr0, %rax");       \
    asm volatile("orq $0x80000000, %rax"); \
    asm volatile("movq %rax, %cr0");       \
    asm volatile("ltr %ax"                 \
                 :                         \
                 : "a"(TSS_SELECTOR));

bool timer_fired = false;

static spinlock_t schedlock_t = SPINLOCK_INIT;

const uint64_t quantum = 20;

// Function to create a new process
struct Process create_process(int id, int priority)
{
    struct Process p;
    p.id = id;
    p.priority = priority;
    p.allocated_time = 0;

    return p;
}

// Function to add a new process to the scheduler
void add_process(struct Scheduler *scheduler, struct Process p)
{
    // Increase size of array to store processes
    scheduler->processes = realloc(scheduler->processes, ++scheduler->n * sizeof(struct Process));

    // Add new process to the end of the array
    scheduler->processes[scheduler->n - 1] = p;
}

// Function to remove a process from the scheduler
void remove_process(struct Scheduler *scheduler, int id)
{
    // Find the index of the process to be removed
    int index = -1;
    for (int i = 0; i < scheduler->n; i++)
    {
        if (scheduler->processes[i].id == id)
        {
            index = i;
            break;
        }
    }

    // Return if process is not found
    if (index == -1)
        return;

    // Shift all subsequent processes one position to the left
    for (int i = index; i < scheduler->n - 1; i++)
        scheduler->processes[i] = scheduler->processes[i + 1];
}

// Function to perform scheduling
void schedule(struct Scheduler *scheduler, int time_quantum)
{
    pic_mask_irq(0);
    // Check if an interrupt has been generated by the PIT
    if (timer_fired)
    {
        // Allocate CPU time to the current process
        struct Process *current = &scheduler->processes[0];
        current->allocated_time += time_quantum;

        // If the current process has been allocated enough time, rotate the processes in the scheduler
        if (current->allocated_time >= 10)
        {
            // Rotate the processes in the scheduler
            for (int i = 0; i < scheduler->n - 1; i++)
                scheduler->processes[i] = scheduler->processes[i + 1];
            scheduler->processes[scheduler->n - 1] = *current;

            // Reset the allocated time for the current process
            current->allocated_time = 0;
        }

        /*        // Check if the current process is requesting a syscall
               if (current->request_syscall)
               {
                   // Handle the syscall request
                   handle_syscall(current);

                   // Reset the syscall request flag
                   current->request_syscall = 0;
               }
        */
        // Reset the interrupt generated flag
        timer_fired = 0;
    }
    pic_unmask_irq(0);
}

// Global variable to store the next available ID
uint64_t next_id = 1;

// Function to generate a new ID
uint64_t generate_id()
{
    // Get the next available ID
    uint64_t id = next_id;

    // Increment the next available ID
    next_id++;

    // Return the new ID
    return id;
}

// Function to fork a process
void fork(struct Scheduler *scheduler, struct Process p)
{
    // Create a new process by copying the original process
    struct Process new_p = p;

    // Assign a new ID to the new process
    new_p.id = generate_id();

    // Add the new process to the scheduler
    add_process(scheduler, new_p);
}
