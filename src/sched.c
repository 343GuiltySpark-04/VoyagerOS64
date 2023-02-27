#include <stdint.h>
#include "include/printf.h"
#include "include/liballoc.h"
#include "include/sched.h"
#include "include/lock.h"
#include "include/gdt.h"
#include "include/pic.h"
#include "include/string.h"
#include "include/idt.h"
#include <stdbool.h>
#include "include/stack_trace.h"
#include "include/KernelUtils.h"

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

bool sched_started = false;

static spinlock_t schedlock_t = SPINLOCK_INIT;

const uint64_t quantum = 5;

const uint64_t quantum_limit = 15;

extern breakpoint();

extern halt();

/**
 * @brief Create a new process with the given parameters
 * @param id ID of the process to create
 * @param priority Priority of the process to create
 * @param mortality Whether or not to mortal the process
 * @param name [ 256 ]
 * @return Pointer to the newly created
 */
struct Process create_process(uint64_t id, uint64_t priority, bool mortality, char name[256])
{

    struct Process p;

    for (int i = 0; i < 256; i++)
    {

        p.name[i] = name[i];
    }

    uint64_t pid = (uint64_t)pid_hash(name);

    p.priority = priority;
    p.allocated_time = 0;
    p.immortal = mortality;
    p.id = pid;

    return p;
}

/**
 * @brief Create a Tube Process.
 * @param high_priority Whether to create a high priority process
 * @param immortal Whether to create an immortal process
 * @param repeat Whether to repeat the process
 * @param name [ 256 ]
 * @return The newly created process or just fucking crash for now
 */
struct tube_process create_tube_process(bool high_priority, bool immortal, bool repeat, char name[256])
{
    spinlock_test_and_acq(&schedlock_t);

    struct tube_process p;

    for (int i = 0; i < 256; i++)
    {

        p.name[i] = name[i];
    }

    uint64_t pid = (uint64_t)pid_hash(name);

    p.allocated_time = 0;
    p.immortal = immortal;
    p.high_priority = high_priority;
    p.repeat = repeat;
    p.id = pid;
    p.exit_code = 0;
    p.syscall = 0;
    // p.parent_id = NULL;

    spinlock_release(&schedlock_t);

    return p;
}

/**
 * @brief High Yield Test. Prints a message to the standard output followed by a break.
 * @return void ( no return value
 */
void high_yield()
{

    printf_("%s\n", "High Yield Test!");
}

/**
 * @brief Exit Tube Function This function is called when the program is finished. In this case we print a message to the user
 */
void exit_tube()
{

    printf_("%s\n", "Exit Test!");
}

/**
 * @brief Exit Tube ISR Register
 */
void exit_reg()
{

    uint8_t vector = idt_allocate_vector();

    // Kernel Panic if vectors are exausted.
    if (vector == NULL)
    {

        printf_("%s\n", "!!!Kernel Panic!!!");
        printf_("%s", "IDT VECTORS EXUSTED!");
        printf_("%s\n", "!!!Kernel Panic!!!");
        halt();
    }

    isr_delta[vector] = exit_tube;

    printf_("%s", "ISR Delta Data: ");
    printf_("0x%llx\n", isr_delta[vector]);

    printf_("%s", "Allocated High Yield ISR at Vector: ");
    printf_("%i\n", vector);

    idt_set_descriptor(vector, isr_stub_table[vector], IDT_DESCRIPTOR_EXTERNAL, 001);
}

/**
 * @brief High Yield ISR Register. Allocates a new IDT vector and sets it as the high yield ISR.
 * @return 0 on success non - zero on
 */
void high_yield_reg()
{

    uint8_t vector = idt_allocate_vector();

    if (vector == NULL)
    {

        printf_("%s\n", "!!!Kernel Panic!!!");
        printf_("%s", "IDT VECTORS EXUSTED!");
        printf_("%s\n", "!!!Kernel Panic!!!");
        halt();
    }

    isr_delta[vector] = high_yield;

    printf_("%s", "ISR Delta Data: ");
    printf_("0x%llx\n", isr_delta[vector]);

    printf_("%s", "Allocated High Yield ISR at Vector: ");
    printf_("%i\n", vector);

    idt_set_descriptor(vector, isr_stub_table[vector], IDT_DESCRIPTOR_EXTERNAL, 001);
}

/**
 * @brief Initialize the scheduler. This is called at boot time to set up the high - yield interrupts
 */
void init_sched(struct standby_tube *s_tube, struct active_tube *a_tube, struct hot_tube *h_tube)
{

    high_yield_reg();
    high_yield_int();
    exit_reg();
    asm volatile("int $51");

    uint64_t size = (1 + sizeof(struct tube_process)) * 3;

    uint64_t hot_size = (1 + sizeof(struct tube_process));

    h_tube->processes = malloc(hot_size);

    s_tube->processes = malloc(size);

    a_tube->processes = malloc(size);
}

// Generate a PID from hashing the process name
/**
 * @brief This function hashes a name using FNV
 * @param name [ 256 ]
 * @return A 64 - bit hash
 */
uint64_t pid_hash(char name[256])
{

    uint64_t input = str2int2(&name);

    // Use the FNV-1a algorithm to generate a 64-bit hash value for the input
    const uint64_t FNV_PRIME = 16777619;
    const uint64_t FNV_OFFSET_BASIS = 0x811c9dc5;
    uint64_t hash = FNV_OFFSET_BASIS;
    for (int i = 0; i < sizeof(input); i++)
    {
        hash ^= (input & 0xff); // XOR with lower 8 bits of input
        input >>= 8;            // Shift right by 8 bits
        hash *= FNV_PRIME;      // Multiply by prime number
    }

    return hash;
}

// Function to add a new process to the scheduler
/**
 * @brief Add a process to the end of the list
 * @param * scheduler
 * @param p Pointer to the process to
 */
void add_process(struct Scheduler *scheduler, struct Process p)
{
    // Increase size of array to store processes
    scheduler->processes = realloc(scheduler->processes, ++scheduler->n * sizeof(struct Process));

    // Add new process to the end of the array
    scheduler->processes[scheduler->n - 1] = p;

    scheduler->current_process++;
}

/**
 * @brief Add a standby process to a standby tube
 * @param tube
 * @param p The process to add to the
 */
void add_tube_process(struct standby_tube *tube, struct tube_process p)
{

    spinlock_test_and_acq(&schedlock_t);

    printf_("%s\n", "New process data as follows.");
    printf_("%s\n", "----------------------------------------");
    printf("%s", "Process Name: ");
    printf_("%s\n", p.name);
    printf_("%s", "PID: ");
    printf_("%u\n", p.id);
    printf_("%s", "Repeat flag: ");
    printf_("%u\n", p.repeat);
    printf_("%s", "Immortal flag: ");
    printf_("%u\n", p.immortal);
    printf_("%s", "The size of the process is: ");
    printf_("%u", sizeof(p));
    printf_("%s", " or ");
    printf_("0x%llx\n", sizeof(p));
    printf_("%s\n", "----------------------------------------");

    tube->processes = realloc(tube->processes, ++tube->process_count * sizeof(struct tube_process));

    tube->processes[tube->process_count - 1] = p;

    tube->current_standby++;

    uint8_t index = tube->process_count - 1;

    printf_("%s\n", "New process data (in the tube) as follows.");
    printf_("%s\n", "----------------------------------------");
    printf("%s", "Process Name: ");
    printf_("%s\n", tube->processes[index].name);
    printf_("%s", "PID: ");
    printf_("%u\n", tube->processes[index].id);
    printf_("%s", "Repeat flag: ");
    printf_("%u\n", tube->processes[index].repeat);
    printf_("%s", "Immortal flag: ");
    printf_("%u\n", tube->processes[index].immortal);
    printf_("%s", "The size of the process is: ");
    printf_("%u", sizeof(tube->processes[index]));
    printf_("%s", " or ");
    printf_("0x%llx\n", sizeof(tube->processes[index]));
    printf_("%s\n", "----------------------------------------");

    spinlock_release(&schedlock_t);
}

/**
 * @brief Add a process to a tube. This is a blocking call and will block until the process is added to the tube
 * @param tube
 * @param p pointer to the process to add to the active
 */
void add_active_tube_process(struct active_tube *tube, struct tube_process p)
{

    spinlock_test_and_acq(&schedlock_t);

    printf_("%s\n", "New process data as follows.");
    printf_("%s\n", "----------------------------------------");
    printf("%s", "Process Name: ");
    printf_("%s\n", p.name);
    printf_("%s", "PID: ");
    printf_("%u\n", p.id);
    printf_("%s", "Repeat flag: ");
    printf_("%u\n", p.repeat);
    printf_("%s", "Immortal flag: ");
    printf_("%u\n", p.immortal);
    printf_("%s", "The size of the process is: ");
    printf_("%u", sizeof(p));
    printf_("%s", " or ");
    printf_("0x%llx\n", sizeof(p));
    printf_("%s\n", "----------------------------------------");

    tube->processes = realloc(tube->processes, ++tube->process_count * sizeof(struct tube_process));

    tube->processes[tube->process_count - 1] = p;

    tube->current_active++;

    uint8_t index = tube->process_count - 1;

    printf_("%s\n", "New process data (in the tube) as follows.");
    printf_("%s\n", "----------------------------------------");
    printf("%s", "Process Name: ");
    printf_("%s\n", tube->processes[index].name);
    printf_("%s", "PID: ");
    printf_("%u\n", tube->processes[index].id);
    printf_("%s", "Repeat flag: ");
    printf_("%u\n", tube->processes[index].repeat);
    printf_("%s", "Immortal flag: ");
    printf_("%u\n", tube->processes[index].immortal);
    printf_("%s", "The size of the process is: ");
    printf_("%u", sizeof(tube->processes[index]));
    printf_("%s", " or ");
    printf_("0x%llx\n", sizeof(tube->processes[index]));
    printf_("%s\n", "----------------------------------------");

    spinlock_release(&schedlock_t);
}

/**
 * @brief [DEPRECIATED] Shift active_tube to standby_tube
 * @param standby_tube [ IN ] number of standby processes
 * @param active_tube [ IN ] number of active
 */
void shift_active(struct standby_tube *s_tube, struct active_tube *a_tube)
{

    spinlock_test_and_acq(&schedlock_t);

    uint64_t space_inc = (a_tube->process_count + 1) * sizeof(struct tube_process);

    a_tube->processes = realloc(a_tube->processes, space_inc);

    a_tube->processes[a_tube->process_count] = s_tube->processes[s_tube->process_count - 1];

    a_tube->current_active++;

    s_tube->processes = realloc(s_tube->processes, --s_tube->process_count * sizeof(struct tube_process));

    s_tube->current_standby--;

    spinlock_release(&schedlock_t);
}

/**
 * @brief [DEPRECIATED] standby to the end of the active tube. This is used to ensure that we don't get stuck in the middle of a tube while processing jobs
 *
 * @param standby_tube - [ IN ] Standby tube to shift to
 * @param active_tube - [ IN ] Active tube to shift from
 */
void shift_standby(struct standby_tube *standby_tube, struct active_tube *active_tube)
{

    spinlock_test_and_acq(&schedlock_t);

    // Remove the active process from the active tube if repeat is false
    if (active_tube->processes[active_tube->process_count - 1].repeat == false)
    {

        remove_active(&active_tube);

        spinlock_release(&schedlock_t);

        return;
    }

    standby_tube->processes = realloc(standby_tube->processes, ++standby_tube->process_count * sizeof(struct tube_process));

    standby_tube->processes[standby_tube->process_count - 1] = active_tube->processes[active_tube->process_count - 1];

    standby_tube->current_standby++;

    active_tube->processes = realloc(active_tube->processes, --active_tube->process_count * sizeof(struct tube_process));

    active_tube->current_active--;

    spinlock_release(&schedlock_t);
}

/**
 * @brief [DEPRECIATED] an active process from the a active tube.
 *
 * @param tube
 */
void remove_active(struct active_tube *tube)
{

    spinlock_test_and_acq(&schedlock_t);

    // If the process is immortal then fail with error message
    if (tube->processes[tube->process_count - 1].immortal == true)
    {

        printf_("%s\n", "ERROR: ATEMPT TO TERMINATE A IMMORTAL THREAD! ABORTING EXIT!");

        spinlock_release(&schedlock_t);

        return;
    }

    uint8_t exit_code = tube->processes[tube->process_count - 1].exit_code;

    tube->processes = realloc(tube->processes, --tube->process_count * sizeof(struct tube_process));

    tube->current_active--;

    spinlock_release(&schedlock_t);

    return exit_code;
}

// Function to remove a process from the scheduler
/**
 * @brief Remove a process from the list
 * @param * scheduler
 * @param id ID of the process to be removed
 * @return void Removal is done in - place
 */
void remove_process(struct Scheduler *scheduler, uint64_t id)
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

    scheduler->current_process--;

    scheduler->processes = realloc(scheduler->processes, --scheduler->n * sizeof(struct Process));
}

// Function to perform scheduling
/**
 * @brief Schedules a process to run.
 * @param * scheduler
 * @param time_quantum Time in milliseconds to reserve
 */
void schedule(struct Scheduler *scheduler, uint64_t time_quantum)
{
    pic_mask_irq(0);
    // Check if an interrupt has been generated by the PIT

    printf_("%s", "The size of the processes array is: ");
    printf_("%u\n", scheduler->n);

    if (timer_fired == true)
    {
        // Allocate CPU time to the current process
        struct Process *current = &scheduler->processes[scheduler->current_process];
        current->allocated_time += time_quantum;

        // If the current process has been allocated enough time, rotate the processes in the scheduler
        if (current->allocated_time >= quantum_limit)
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
        timer_fired = false;
    }

    printf_("%s\n", "succes");

    pic_unmask_irq(0);
}

/**
 * @brief Schedule a tubes. This is called to manage processes.
 * @param standby The tube to store processes on standby.
 * @param active The tube to store and handle actively qeued processes.
 * @param quantum The time in milliseconds to run the process for.
 */
void tube_schedule(struct standby_tube *standby, struct active_tube *active, struct hot_tube *hot, uint64_t quantum)
{

    pic_mask_irq(0);

    spinlock_acquire(&schedlock_t);

    uint8_t index;

    if (k_mode.sched_debug == 1)
    {

        printf_("%s", "Size of the Standby Tube is: ");
        printf_("%u\n", standby->process_count);
        printf_("%s", "The size of the Active Tube is: ");
        printf_("%u\n", active->process_count);
        printf_("%s", "The size of a process is: ");
        printf_("%u", sizeof(struct tube_process));
        printf_("%s", " or ");
        printf_("0x%llx\n", sizeof(struct tube_process));
        printf_("%s\n", "----------------------------------------");
    }

    // Initalzies the algorithem on first run
    if (sched_started == false)
    {

        index = active->process_count - 1;

        if (k_mode.sched_debug == 1)
        {

            printf_("%s\n", "New process data (in the tube) as follows.");
            printf_("%s\n", "----------------------------------------");
            printf("%s", "Process Name: ");
            printf_("%s\n", active->processes[index].name);
            printf_("%s", "PID: ");
            printf_("%u\n", active->processes[index].id);
            printf_("%s", "Repeat flag: ");
            printf_("%u\n", active->processes[index].repeat);
            printf_("%s", "Immortal flag: ");
            printf_("%u\n", active->processes[index].immortal);
            printf_("%s", "The size of the process is: ");
            printf_("%u", sizeof(active->processes[index]));
            printf_("%s", " or ");
            printf_("0x%llx\n", sizeof(active->processes[index]));
            printf_("%s\n", "----------------------------------------");

            printf_("%s", "Size of the Standby Tube is: ");
            printf_("%u\n", standby->process_count);
            printf_("%s", "The size of the Active Tube is: ");
            printf_("%u\n", active->process_count);
            printf_("%s", "The size of a process is: ");
            printf_("%u", sizeof(struct tube_process));
            printf_("%s", " or ");
            printf_("0x%llx\n", sizeof(struct tube_process));
            printf_("%s\n", "----------------------------------------");
        }

        sched_started = true;
    }

    uint64_t a_cap = active->process_count - 1;

    uint64_t s_cap = standby->process_count - 1;

    uint64_t a_index = 0;

    uint64_t s_index = 0;

    if (k_mode.sched_debug == 1)
    {

        printf_("%s\n", "INFO: Data for all active tube processes as follows: ");
        printf_("%s\n", "-------------------------------------------------------");

        while (a_index <= a_cap)
        {

            struct tube_process *a_spec = &active->processes[a_index];

            printf_("%s\n", "New process data as follows.");
            printf_("%s\n", "----------------------------------------");
            printf_("%s", "Active Index Number: ");
            printf_("%u\n", a_index);
            printf("%s", "Process Name: ");
            printf_("%s\n", a_spec->name);
            printf_("%s", "PID: ");
            printf_("%u\n", a_spec->id);
            printf_("%s", "Repeat flag: ");
            printf_("%u\n", a_spec->repeat);
            printf_("%s", "Immortal flag: ");
            printf_("%u\n", a_spec->immortal);
            printf_("%s", "The size of the process is: ");
            printf_("%u", sizeof(*a_spec));
            printf_("%s", " or ");
            printf_("0x%llx\n", sizeof(*a_spec));
            printf_("%s\n", "----------------------------------------");

            a_index++;
        }

        printf_("%s\n", "INFO: Data for all standby tube processes as follows: ");
        printf_("%s\n", "-------------------------------------------------------");

        while (s_index <= s_cap)
        {

            struct tube_process *s_spec = &standby->processes[s_index];

            printf_("%s\n", "New process data as follows.");
            printf_("%s\n", "----------------------------------------");
            printf_("%s", "Standby Index Number: ");
            printf_("%u\n", s_index);
            printf("%s", "Process Name: ");
            printf_("%s\n", s_spec->name);
            printf_("%s", "PID: ");
            printf_("%u\n", s_spec->id);
            printf_("%s", "Repeat flag: ");
            printf_("%u\n", s_spec->repeat);
            printf_("%s", "Immortal flag: ");
            printf_("%u\n", s_spec->immortal);
            printf_("%s", "The size of the process is: ");
            printf_("%u", sizeof(*s_spec));
            printf_("%s", " or ");
            printf_("0x%llx\n", sizeof(*s_spec));
            printf_("%s\n", "----------------------------------------");

            s_index++;
        }
    }
    // if timer fired then shift processes.
    if (timer_fired == true)
    {

        struct tube_process *current = &active->processes[active->process_count - 1];
        current->allocated_time += quantum;

        // check for syscall requests.
        if (current->syscall != 0)
        {

            // find the current syscall and service it.
            switch (current->syscall)
            {

            case SYSCALL_YIELD:
                asm volatile("int $50");
                break;
            case SYSCALL_FORK:
                asm volatile("int $52");
                break;
            case SYSCALL_EXIT:
                asm volatile("int $51");
                break;
            default:
                printf_("%s", "ERROR: Invalid Syscall: ");
                printf_("%u\n", current->syscall);
                break;
            }
        }

        // shift standby and active processes.
        if (current->allocated_time >= quantum_limit)
        {

            current->allocated_time = 0;

            printf_("%s", "Size of the Standby Tube is: ");
            printf_("%u\n", standby->process_count);
            printf_("%s", "The size of the Active Tube is: ");
            printf_("%u\n", active->process_count);
            printf_("%s", "The size of a process is: ");
            printf_("%u", sizeof(struct tube_process));
            printf_("%s", " or ");
            printf_("0x%llx\n", sizeof(struct tube_process));
            printf_("%s\n", "----------------------------------------");

            // Remove the active process from the active tube if repeat is false
            if (active->processes[active->process_count - 1].repeat == false)
            {

                // If the process is immortal then fail with error message
                if (active->processes[active->process_count - 1].immortal == true)
                {

                    printf_("%s\n", "ERROR: ATEMPT TO TERMINATE A IMMORTAL THREAD! ABORTING EXIT!");
                }

                active->prev_exit_code = active->processes[active->process_count - 1].exit_code;

                active->processes = realloc(active->processes, (--active->process_count) * sizeof(struct tube_process));

                active->current_active--;

                printf_("%s\n", "!!Remove finished!!");
                printf_("%s", "Size of the Standby Tube is: ");
                printf_("%u\n", standby->process_count);
                printf_("%s", "The size of the Active Tube is: ");
                printf_("%u\n", active->process_count);
                printf_("%s", "The size of a process is: ");
                printf_("%u", sizeof(struct tube_process));
                printf_("%s", " or ");
                printf_("0x%llx\n", sizeof(struct tube_process));
                printf_("%s\n", "----------------------------------------");
            }
            else if (current->id == NULL || 0)
            {

                printf_("%s\n", "!!!Kernel Panic!!!");
                printf_("%s", "PROCESS WITH NULL DATA!");
                printf_("%s\n", "!!!Kernel Panic!!!");
                halt();
            }
            else
            {

                hot->processes = realloc(hot->processes, (++hot->process_count) * sizeof(struct tube_process));

                hot->processes[hot->process_count - 1] = active->processes[0];

                hot->processes = realloc(hot->processes, (++hot->process_count) * sizeof(struct tube_process));

                hot->processes[hot->process_count - 1] = standby->processes[0];

                uint64_t h_cap = hot->process_count - 1;

                uint64_t h_index = 0;

                printf_("%s\n", "INFO: Data for all hot (debug) tube processes as follows: ");
                printf_("%s\n", "-------------------------------------------------------");

                while (h_index <= h_cap)
                {

                    struct tube_process *h_spec = &hot->processes[h_index];

                    printf_("%s\n", "Hot process data as follows.");
                    printf_("%s\n", "----------------------------------------");
                    printf_("%s", "Hot Index Number: ");
                    printf_("%u\n", h_index);
                    printf("%s", "Process Name: ");
                    printf_("%s\n", h_spec->name);
                    printf_("%s", "PID: ");
                    printf_("%u\n", h_spec->id);
                    printf_("%s", "Repeat flag: ");
                    printf_("%u\n", h_spec->repeat);
                    printf_("%s", "Immortal flag: ");
                    printf_("%u\n", h_spec->immortal);
                    printf_("%s", "The size of the process is: ");
                    printf_("%u", sizeof(*h_spec));
                    printf_("%s", " or ");
                    printf_("0x%llx\n", sizeof(*h_spec));
                    printf_("%s\n", "----------------------------------------");

                    h_index++;
                }

                active->processes = realloc(active->processes, (++active->process_count) * sizeof(struct tube_process));

                active->processes[active->process_count - 1] = standby->processes[0];

                standby->processes = realloc(standby->processes, (++standby->process_count) * sizeof(struct tube_process));

                standby->processes[standby->process_count - 1] = active->processes[0];

                for (uint64_t i = active->process_count - 1; i > 0; i--)
                {

                    active->processes[i - 1] = active->processes[i];
                }

                for (uint64_t i = standby->process_count - 1; i > 0; i--)
                {

                    standby->processes[i - 1] = standby->processes[i];
                }

                standby->processes = realloc(standby->processes, (standby->process_count - 1) * sizeof(struct tube_process));

                active->processes = realloc(active->processes, (active->process_count - 1) * sizeof(struct tube_process));

                /*                 standby->processes = realloc(standby->processes, (++standby->process_count) * sizeof(struct tube_process));

                                standby->processes[standby->process_count - 1] = active->processes[active->process_count - 1];

                                standby->current_standby++;

                                active->processes = realloc(active->processes, (--active->process_count) * sizeof(struct tube_process));

                                active->current_active--;

                                active->processes = realloc(active->processes, (++active->process_count) * sizeof(struct tube_process));

                                active->processes[active->process_count - 1] = standby->processes[standby->process_count - 1];

                                active->current_active++; */

                a_cap = active->process_count - 1;

                s_cap = standby->process_count - 1;

                a_index = 0;

                s_index = 0;

                printf_("%s\n", "INFO: Data for all active tube processes as follows: ");
                printf_("%s\n", "-------------------------------------------------------");

                while (a_index <= a_cap)
                {

                    struct tube_process *a_spec = &active->processes[a_index];

                    printf_("%s\n", "New process data as follows.");
                    printf_("%s\n", "----------------------------------------");
                    printf_("%s", "Active Index Number: ");
                    printf_("%u\n", a_index);
                    printf("%s", "Process Name: ");
                    printf_("%s\n", a_spec->name);
                    printf_("%s", "PID: ");
                    printf_("%u\n", a_spec->id);
                    printf_("%s", "Repeat flag: ");
                    printf_("%u\n", a_spec->repeat);
                    printf_("%s", "Immortal flag: ");
                    printf_("%u\n", a_spec->immortal);
                    printf_("%s", "The size of the process is: ");
                    printf_("%u", sizeof(*a_spec));
                    printf_("%s", " or ");
                    printf_("0x%llx\n", sizeof(*a_spec));
                    printf_("%s\n", "----------------------------------------");

                    a_index++;
                }

                printf_("%s\n", "INFO: Data for all standby tube processes as follows: ");
                printf_("%s\n", "-------------------------------------------------------");

                while (s_index <= s_cap)
                {

                    struct tube_process *s_spec = &standby->processes[s_index];

                    printf_("%s\n", "New process data as follows.");
                    printf_("%s\n", "----------------------------------------");
                    printf_("%s", "Standby Index Number: ");
                    printf_("%u\n", s_index);
                    printf("%s", "Process Name: ");
                    printf_("%s\n", s_spec->name);
                    printf_("%s", "PID: ");
                    printf_("%u\n", s_spec->id);
                    printf_("%s", "Repeat flag: ");
                    printf_("%u\n", s_spec->repeat);
                    printf_("%s", "Immortal flag: ");
                    printf_("%u\n", s_spec->immortal);
                    printf_("%s", "The size of the process is: ");
                    printf_("%u", sizeof(*s_spec));
                    printf_("%s", " or ");
                    printf_("0x%llx\n", sizeof(*s_spec));
                    printf_("%s\n", "----------------------------------------");

                    s_index++;
                }

                // shift all processes down the tube
                /*
                                standby->processes = realloc(standby->processes, (--standby->process_count) * sizeof(struct tube_process));

                                standby->current_standby--;
                 */
                printf_("%s\n", "!!L1 marker!!");
                printf_("%s", "Size of the Standby Tube is: ");
                printf_("%u\n", standby->process_count);
                printf_("%s", "The size of the Active Tube is: ");
                printf_("%u\n", active->process_count);
                printf_("%s", "The size of a process is: ");
                printf_("%u", sizeof(struct tube_process));
                printf_("%s", " or ");
                printf_("0x%llx\n", sizeof(struct tube_process));
                printf_("%s\n", "----------------------------------------");
            }
        }

        timer_fired = false;

        printf_("%s\n", "Timer reset!");
    }

    spinlock_release(&schedlock_t);
    printf_("%s\n", "Spinlock realeased!");

    pic_unmask_irq(0);

    printf_("%s\n", "IRQ Unmasked!");
}

// Global variable to store the next available ID
uint64_t next_id = 2;

// Function to generate a seed ID
/**
 * @brief Generates a new ID for the object
 * @return The ID of the new
 */
uint64_t generate_id()
{
    // Get the next available ID
    uint64_t id = next_id;

    // Increment the next available ID
    next_id++;

    // Return the new ID
    return id;
}

// Function to create a new child process
/**
 * @brief Fork a process from the current process
 * @param * scheduler
 */
void fork(struct Scheduler *scheduler)
{

    uint64_t parent_id = scheduler->processes[scheduler->current_process - 1].id;
    uint64_t parent_index = scheduler->current_process - 1;
    char parent_name;

    parent_name = scheduler->processes[parent_index].name;

    // Create a new process with the the ID being a hash generated from the parent PID
    uint64_t id = pid_hash(parent_id);
    if (id == scheduler->processes[parent_index].id)
    {

        id++;
    }
    printf_("Parent ID: %u\n", parent_id);
    printf_("Child ID: %u\n", id);
    struct Process child = create_process(id, 2, false, parent_name);

    // Add the new process to the scheduler
    add_process(scheduler, child);
}

// Function to exit the current process
/**
 * @brief This function is called when the scheduler is shutting down.
 * @param * scheduler
 * @return Returns nothing. The scheduler must be stopped
 */
void exit(struct Scheduler *scheduler)
{
    // Get the ID of the current process
    int id = scheduler->processes[scheduler->current_process].id;

    // Check if the immortal flag is present
    bool flag = scheduler->processes[0].immortal;
    if (flag == true)
    {

        // spinlock_acquire(&schedlock_t);

        printf_("%s\n", "ERROR: ATEMPT TO TERMINATE A CORE SYSTEM THREAD! ABORTING EXIT!");
        printf_("%s", "Current PID: ");
        printf_("%i\n", id);

        return;
    }

    // Remove the current process from the scheduler
    remove_process(scheduler, id);

    // printf_("%s\n", "DONE");

    return;
}
