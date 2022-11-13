#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include "include/cpu.h"
#include "include/registers.h"
#include "include/gdt.h"
#include "include/tss.h"
#include "include/idt.h"

bool sysenter;