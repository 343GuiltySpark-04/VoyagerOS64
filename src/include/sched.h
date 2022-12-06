#include <stdint.h>

typedef uint64_t pid_t;

// data structure to represent a process
typedef struct
{
    pid_t pid;
    uint64_t arrival_time;
    uint64_t remaining_time;
} process_t;

// data structure to represent a queue of processes
typedef struct
{
    process_t *processes;
    uint64_t size;
    uint64_t capacity;
} queue_t;

queue_t *create_queue(uint64_t capacity);
void free_queue(queue_t *queue);
void enqueue(queue_t *queue, process_t process);
process_t dequeue(queue_t *queue);