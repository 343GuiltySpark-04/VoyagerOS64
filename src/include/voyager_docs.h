/**
 * @file voyager_docs.h
 * @brief Doxygen landing pages and subsystem groups for VoyagerOS64.
 *
 * This file intentionally contains documentation only. It is scanned by
 * Doxygen because the configured input recursively includes src/include.
 */
#pragma once

/**
 * @mainpage VoyagerOS64 Kernel Documentation
 *
 * VoyagerOS64 is a hobby x86-64 kernel. The 0.0.5 development line focuses on
 * replacing several overlapping memory-management experiments with one
 * well-defined bootstrap/PMM/heap chain and on restoring a small, trustworthy
 * cooperative scheduler.
 *
 * @section architecture Current 0.0.5 architecture
 *
 * The boot-time ownership chain is:
 *
 * @code{.unparsed}
 * Limine
 *   -> early_alloc
 *   -> paging_bootstrap
 *   -> neo_framealloc
 *   -> kmalloc
 *   -> scheduler / drivers / VSH
 * @endcode
 *
 * The most important invariant is that physical and virtual addresses are not
 * interchangeable. Physical-memory management deals in physical frames.
 * Paging creates virtual mappings. Kernel code reaches physical frames through
 * the HHDM or another explicit mapping.
 *
 * @section scheduler_model Scheduler model
 *
 * Version 0.0.5 uses cooperative round-robin scheduling. Tasks voluntarily
 * yield by calling schedule(). The PIT preemption gate remains disabled on
 * purpose; switch_to() is an ordinary-context stack switch and is not an IRQ
 * return path.
 *
 * @section release_boundaries Deliberate 0.0.5 boundaries
 *
 * The following are not complete 0.0.5 facilities: preemptive scheduling,
 * task exit/reaping, wait semantics, Unix-style fork/exec, per-process address
 * spaces, scheduler-integrated FPU/XSAVE state, SMP scheduling, and APIC-based
 * interrupt routing as the primary path.
 *
 * @section historical_code Historical implementations
 *
 * Several older allocators, VMM experiments, the former tube/stream layer, and
 * C++ allocation/locking experiments remain in-tree for archaeology and design
 * history. GNUmakefile explicitly excludes those implementations from the
 * active kernel link. Their presence in the source tree does not make them part
 * of the 0.0.5 runtime architecture.
 *
 * @section qualification Runtime qualification
 *
 * The Stage-2 memory stack has survived direct PMM round-trips, allocation
 * boundary tests, fragmentation/split/coalesce tests, krealloc preservation,
 * multi-megabyte heap growth, deterministic randomized torture, and repeated
 * same-boot runs. The cooperative scheduler has also survived dynamic run-queue
 * growth into the low twenties of runnable tasks during soak testing.
 */

/** @defgroup boot Boot and Kernel Bring-up
 *  @brief Entry, bootloader handoff, subsystem ordering, and ownership transfer.
 */

/** @defgroup memory Memory Management
 *  @brief Bootstrap allocation, physical-frame ownership, and kernel heap.
 */

/** @defgroup early_alloc Early Physical Allocator
 *  @ingroup memory
 *  @brief One-way bootstrap allocator used before the permanent PMM is ready.
 */

/** @defgroup pmm Physical Memory Manager
 *  @ingroup memory
 *  @brief Ownership and allocation of 4 KiB physical frames.
 */

/** @defgroup heap Kernel Heap
 *  @ingroup memory
 *  @brief General-purpose kernel virtual-memory allocation after bring-up.
 */

/** @defgroup paging Paging and Address Translation
 *  @brief x86-64 page-table construction, mappings, CR3 ownership, and HHDM use.
 */

/** @defgroup interrupts Interrupts and Descriptor Tables
 *  @brief GDT, IDT, PIC/PIT, interrupt dispatch, and related low-level state.
 */

/** @defgroup scheduler Scheduler and Processes
 *  @brief Cooperative 0.0.5 task creation and round-robin context switching.
 */

/** @defgroup drivers Device Drivers
 *  @brief Hardware drivers exposed to the rest of the kernel.
 */

/** @defgroup keyboard PS/2 Keyboard
 *  @ingroup drivers
 *  @brief IRQ-driven keyboard input and the shell-facing character FIFO.
 */

/** @defgroup terminal Terminal and Console
 *  @brief Boot console, framebuffer terminal, serial diagnostics, and output ownership.
 */

/** @defgroup shell Voyager Shell (VSH)
 *  @brief Cooperative interactive shell and built-in kernel commands.
 */

/** @defgroup diagnostics Diagnostics and Qualification
 *  @brief Built-in validation, stress tests, and low-level debug instrumentation.
 */

/** @defgroup time Timekeeping
 *  @brief RTC, PIT-derived timing, date formatting, and system time services.
 */

/** @defgroup deprecated_code Retired / Historical Code
 *  @brief Preserved experiments that are not linked into the active 0.0.5 kernel.
 *
 *  The build explicitly retires older allocator/VMM/stream implementations.
 *  When reading the source tree, prefer the active Stage-2 interfaces documented
 *  under @ref memory and @ref paging rather than assuming every historical file
 *  participates in the current runtime.
 */
