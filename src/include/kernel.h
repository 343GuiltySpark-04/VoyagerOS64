/**
 * @file kernel.h
 * @brief Global kernel bring-up state shared by low-level subsystems.
 * @ingroup boot
 */
#pragma once
#ifndef _KERNEL_H
#define _KERNEL_H

#include "limine.h"
#include <stdint.h>

/**
 * @brief Console-routing state used during the bootloader-to-Voyager handoff.
 *
 * Boot code changes this while output ownership moves from early boot services
 * to Voyager's serial/framebuffer terminal. Code running after Voyager installs
 * its own CR3 must not assume Limine terminal callbacks remain callable.
 */
extern uint32_t bootspace;

/** @brief Global kernel error/diagnostic mode state. */
extern uint8_t kerror_mode;

/** @brief Nonzero once the main kernel initialization sequence is complete. */
extern uint8_t init_done;

/**
 * @brief Limine terminal request used only by the early boot-console phase.
 *
 * @warning The request/response data may remain readable after CR3 handoff, but
 * the bootloader's terminal callback code is not part of Voyager's guaranteed
 * post-handoff address space. Use Voyager-owned output facilities afterwards.
 */
extern volatile struct limine_terminal_request early_term;

#endif
