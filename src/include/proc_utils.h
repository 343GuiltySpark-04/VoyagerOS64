/**
 * Copyright (c) 2026 Tristan Adams
 *
 * This software is released under the MIT License.
 * https://opensource.org/licenses/MIT
 */

/**
 * @file proc_utils.h
 * @brief Small process/scheduler diagnostic tasks.
 * @ingroup diagnostics
 */
#pragma once

/**
 * @brief Cooperative scheduler soak worker.
 *
 * The VSH `soak` command creates another process running this entry point.
 * Workers identify themselves by PID, repeatedly prove liveness, and yield back
 * into the cooperative round-robin scheduler. Because 0.0.5 has no task-exit
 * path, a soak worker is intentionally long-lived until the system halts.
 */
void soak(void);
