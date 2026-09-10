/**
 * @file lapic.h
 * @brief Local APIC experimental interface.
 * @ingroup interrupts
 *
 * MADT/APIC discovery code exists in the 0.0.5 tree, but the qualified release
 * still uses the legacy PIC path as its primary interrupt route. Treat these
 * routines as groundwork for later APIC/SMP work rather than a 0.0.5 scheduler
 * dependency.
 */
#ifndef _DEV__LAPIC_H
#define _DEV__LAPIC_H

#include <stdint.h>

/** @brief Initialize the current CPU's Local APIC support path. */
void lapic_init(void);

/** @brief Send end-of-interrupt acknowledgement to the Local APIC. */
void lapic_eoi(void);

/**
 * @brief Arm the experimental Local APIC timer in one-shot mode.
 * @param us Requested delay in microseconds.
 * @param function Callback pointer stored for the timer interrupt path.
 */
void lapic_timer_oneshot(uint32_t us, void *function);

/**
 * @brief Send an inter-processor interrupt to a Local APIC ID.
 * @param lapic_id Destination Local APIC identifier.
 * @param vec Interrupt vector to send.
 */
void lapic_send_ipi(uint32_t lapic_id, uint32_t vec);

/** @brief Calibrate the Local APIC timer against the current timing source. */
void lapic_timer_calibrate(void);

/**
 * @brief Return the current processor's Local APIC ID.
 * @return Local APIC identifier reported by the current implementation.
 */
uint32_t lapic_get_id(void);

#endif