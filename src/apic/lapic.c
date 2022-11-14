#include <stddef.h>
#include <stdint.h>
#include "../include/apic/lapic.h"
#include "../include/idt.h"
#include "../include/cpu.h" 
#include "../include/cpuUtils.h"
#include "../include/global_defs.h"
#include "../include/paging/paging.h"
#include "../include/printf.h"
#include "../include/registers.h"
#include "../include/time.h"

#define LAPIC_REG_ID 0x20 // LAPIC ID
#define LAPIC_REG_EOI 0x0b0 // End of interrupt
#define LAPIC_REG_SPURIOUS 0x0f0
#define LAPIC_REG_CMCI 0x2f0 // LVT Corrected machine check interrupt
#define LAPIC_REG_ICR0 0x300 // Interrupt command register
#define LAPIC_REG_ICR1 0x310
#define LAPIC_REG_LVT_TIMER 0x320
#define LAPIC_REG_TIMER_INITCNT 0x380 // Initial count register
#define LAPIC_REG_TIMER_CURCNT 0x390 // Current count register
#define LAPIC_REG_TIMER_DIV 0x3e0
#define LAPIC_EOI_ACK 0x00

static uint8_t timer_vec = 32;

static inline uint32_t lapic_read(uint32_t reg) {
    return *((volatile uint32_t *)((uintptr_t)0xfee00000 + HIGHER_HALF_MEMORY_OFFSET + reg));
}

static inline void lapic_write(uint32_t reg, uint32_t val) {
    *((volatile uint32_t *)((uintptr_t)0xfee00000 + HIGHER_HALF_MEMORY_OFFSET + reg)) = val;
}

static inline void lapic_timer_stop(void) {
    lapic_write(LAPIC_REG_TIMER_INITCNT, 0);
    lapic_write(LAPIC_REG_LVT_TIMER, 1 << 16);
}

static void lapic_timer_handler(int vector, struct cpu_ctx *ctx) {
    lapic_eoi();
    if (this_cpu()->timer_function != NULL) {
        this_cpu()->timer_function(vector, ctx);
    }
}
