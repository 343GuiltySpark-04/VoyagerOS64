/**
 * @file serial.h
 * @brief Serial-console and timestamped diagnostic output helpers.
 * @ingroup terminal
 */
#pragma once
#ifndef SERIAL_H
#define SERIAL_H

#include <stdint.h>

/**
 * @brief Emit one character through the low-level debug serial path.
 * @param c Character to transmit.
 *
 * This primitive is useful during early boot and for diagnostics that must not
 * depend on framebuffer-terminal state.
 */
extern void serial_debug(char c);

/**
 * @brief Read the timestamp source used by serial diagnostics.
 * @return Current diagnostic timestamp value.
 */
extern uint64_t get_ts(void);

/** @brief Write a NUL-terminated string to the serial console. */
void serial_print(const char *str);
/** @brief Write a NUL-terminated string followed by a line ending. */
void serial_print_line(const char *str);
/** @brief Write a diagnostic message prefixed with the current timestamp. */
void serial_print_with_ts(const char *msg);

#endif
