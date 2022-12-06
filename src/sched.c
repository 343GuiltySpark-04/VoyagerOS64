#include <stdint.h>
#include "include/printf.h"
#include "include/liballoc.h"
#include "include/sched.h"

// create a new queue
queue_t *create_queue(uint64_t capacity)
{
    queue_t *queue = (queue_t *)malloc(sizeof(queue_t));
    queue->capacity = capacity;
    queue->size = 0;
    queue->processes = (process_t *)malloc(capacity * sizeof(process_t));
    return queue;
}

// free the memory used by the queue
void free_queue(queue_t *queue)
{
    free(queue->processes);
    free(queue);
}

// add a new process to the queue
void enqueue(queue_t *queue, process_t process)
{
    if (queue->size == queue->capacity)
    {
        printf("Error: queue is full\n");
        return;
    }

    queue->processes[queue->size] = process;
    queue->size++;
}

// remove the next process from the queue
process_t dequeue(queue_t *queue)
{
    if (queue->size == 0)
    {
        printf("Error: queue is empty\n");
        return;
    }

    process_t process = queue->processes[0];
    queue->size--;
}
