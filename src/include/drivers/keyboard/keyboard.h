/**
 * @file keyboard.h
 * @brief PS/2 keyboard IRQ and shell-facing character queue.
 * @ingroup keyboard
 */
#pragma once

/** PS/2 controller data port. */
#define KEYBOARD_DATA_PORT 0x60
/** PS/2 controller status/command port. */
#define KEYBOARD_STATUS_PORT 0x64

/**
 * @brief IRQ1 keyboard handler.
 *
 * Consumes one Set-1 scancode from the controller, ignores break codes, maps
 * supported make codes to characters, and queues them for non-IRQ consumers.
 */
void keyboard_handler(void);

/**
 * @brief Initialize the keyboard input path and make IRQ1 available.
 *
 * The 0.0.5 boot path calls this after the Voyager framebuffer terminal exists,
 * so echoed shell input does not depend on the bootloader terminal callback.
 */
void keyboard_init(void);

/**
 * @brief Legacy single-character keyboard accessor.
 * @return Most recently exposed legacy keyboard character state.
 *
 * Prefer kbd_pop() for VSH and new queued-input code.
 */
char k_getchar(void);

/**
 * @brief Remove one character from the keyboard FIFO.
 * @return Next queued character, or 0 when the queue is empty.
 *
 * The queue is fed by the IRQ handler and consumed by VSH in ordinary task
 * context. An empty queue is not an error; cooperative callers normally yield
 * and try again later.
 */
char kbd_pop(void);
