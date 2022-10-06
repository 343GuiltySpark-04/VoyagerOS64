#pragma once

#include <stdint.h>
#include "global_defs.h"

void idt_set_descriptor(uint8_t vector, void *isr, uint8_t flags);