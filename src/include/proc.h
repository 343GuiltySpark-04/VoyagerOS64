#pragma once

#include <stdint.h>
//#include <signal.h>
#include "global_defs.h"
#include "interrupts.h"
#include "lib/hashmap.h"
#include "lib/vector.h"
#include "lock.h"

extern void proc_yield();

// const int PROCESS_STACK_SIZE = 0x4000;
