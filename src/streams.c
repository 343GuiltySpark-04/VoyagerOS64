#include "include/io.h"
#include "include/kernel.h"
#include "include/lock.h"
#include "include/printf.h"
#include "include/sched.h"
#include "include/string.h"
#include <stddef.h>
#include <stdint.h>

#define IO_STACK_SIZE 4096 - 1

static spinlock_t streamlock_t = SPINLOCK_INIT;
