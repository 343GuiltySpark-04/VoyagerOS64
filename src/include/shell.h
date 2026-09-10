/**
 * @file shell.h
 * @brief Voyager shell (VSH) public interface.
 * @ingroup shell
 *
 * VSH runs as an ordinary cooperative scheduler task. While waiting for
 * keyboard input it yields, allowing the rest of the run queue to continue.
 */
#pragma once

#ifndef _SHELL_H
#define _SHELL_H

/** Initial command-line allocation/growth quantum in bytes. */
#define VSH_CMD_BUFFER_SIZE 100
/** Current shell-interface version. */
#define VSH_VERSION "0.0.1"

/**
 * @brief Main VSH cooperative task.
 *
 * Repeatedly prints a prompt, reads a line from the keyboard FIFO, dispatches
 * the command, frees the line buffer, and yields to the scheduler.
 */
void vsh_loop(void);

/**
 * @brief Read one line from the keyboard FIFO.
 * @return Newly allocated NUL-terminated command string, or NULL if allocation
 *         fails.
 *
 * The buffer is owned by the caller and must be released with kfree(). The
 * function yields with schedule() whenever no keyboard character is available.
 */
char *vsh_readline(void);

/**
 * @brief Dispatch one NUL-terminated VSH command line.
 * @param str Command string to parse.
 */
void cmd_parser(const char *str);

#endif
