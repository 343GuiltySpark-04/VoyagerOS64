# VoyagerOS64 Changelog

## Version 0.0.1 (Was Not Released)

- Added the initial IDT.
- Added the initial GDT.
- Added basic non-PIC interrupt handling.

## Version 0.0.2

- Added working terminal output.
- Added early paging support.
- Added liballoc-based dynamic allocation.

## Version 0.0.3

- Expanded the interrupt system.
- Remapped and enabled the legacy PIC.
- Added the PS/2 keyboard driver.
- Added on-screen error reporting.

## Version 0.0.4

- Major MMU fixes.
- Began multitasking experiments.
- Added verbose CPU feature readout.
- Began SMM and related CPU/platform experiments.
- Added boot-log output through the Limine bootloader terminal.
- Updated the GDT for compatibility with the Limine boot path.
- Added additional kernel panic messages.
- Added a new terminal font.

## Version 0.0.5

0.0.5 is primarily the memory-management and cooperative-scheduler stabilization release. It replaces overlapping allocator experiments in the active runtime with one qualified ownership chain and restores a minimal scheduler, terminal, keyboard, and shell path on top of it.

### Memory and paging

- Added the one-way `early_alloc` bootstrap physical-page allocator.
- Added Voyager-owned bootstrap page tables and CR3 handoff.
- Added the Stage-2 `neo_framealloc` physical memory manager.
- Added the Stage-2 kernel heap with `kmalloc`, `kfree`, and `krealloc`.
- Added the paging frame-supplier handoff from `early_alloc` to `neo_framealloc`.
- Fixed post-CR3 execution by removing reliance on Limine terminal callback code after Voyager installs its own page tables.
- Kept physical-address, HHDM virtual-address, and kernel virtual-address roles explicit in the active memory architecture.
- Retired the old bitmap allocator, Pebble allocator, liballoc glue, older VMM, stream/tube layer, and legacy C++ allocator/locking experiments from the active link while preserving their source for history.

### Memory qualification

- Added the permanent VSH `memtest` diagnostic.
- Added direct PMM allocation/write/verify/free round-trip tests.
- Added heap boundary-size tests.
- Added fragmentation, splitting, and coalescing tests.
- Added `krealloc` preservation and in-place-growth checks.
- Added multi-megabyte heap-growth testing.
- Added deterministic randomized allocation/free/reallocation torture testing.
- Qualified repeated same-boot runs with stable physical-frame high-water behavior and no detected corruption.
- Repeated the full memory test multiple times in one boot without regression.

### Scheduler and processes

- Replaced the broken IRQ-time scheduler experiment with a cooperative round-robin scheduler for 0.0.5.
- Added independent 16 KiB kernel stacks for created processes.
- Added synthetic initial process contexts and a cooperative low-level stack switch.
- Added dynamic process creation while the scheduler is already running.
- Added the VSH `soak` command for scheduler stress testing.
- Qualified dynamic run-queue growth to 22 runnable tasks during long cooperative soak testing.
- Silenced the permanent scheduler heartbeat tasks after qualification while retaining them as cooperative canaries.
- Explicitly kept PIT/IRQ preemption disabled until interrupt-frame-aware preemptive switching is implemented.
- Task exit/reaping, wait semantics, and Unix-style `fork`/`exec` remain future work.

### Terminal, keyboard, and shell

- Restored Voyager-owned framebuffer terminal handoff after the CR3 transition.
- Removed dependence on Limine terminal callbacks after Voyager assumes address-space ownership.
- Added/qualified the keyboard character FIFO between IRQ1 and VSH.
- Restored VSH as a cooperative scheduler task.
- Added dynamic command-buffer growth with `krealloc`.
- Added or retained VSH commands including `time`, `memtest`, `soak`, and `halt`.
- Retained verbose boot diagnostics intentionally for low-level bring-up and debugging.

### Interrupts and platform support

- Added ACPI table discovery and associated failure handling.
- Added MADT parsing groundwork for APIC development.
- Added dynamic interrupt-vector allocation and boot-time vector tests.
- Added the ability to reload GDT/IDT/TSS state used by bring-up code.
- Corrected PIC EOI handling so software/dynamic vectors are not acknowledged as hardware IRQs.
- Retained the legacy PIC path as the qualified primary interrupt route for 0.0.5.

### CPU, diagnostics, and time

- Retained verbose CPUID and boot diagnostics for low-level debugging.
- Added hardware RNG support where available.
- Added standardized panic handling and stack-trace/stack-dump diagnostics.
- Added kernel behavior/debug switches through `k_mode`.
- Improved RTC year/date handling; current QEMU qualification reports the full 2026 date correctly.
- Retained PIT/timekeeping work as partially experimental; precise PIT-derived accounting remains future cleanup.
- SSE/XSAVE capability experiments remain present, but per-task floating-point/XSAVE preservation is not a qualified 0.0.5 scheduler feature.

### Build and documentation

- Cleaned dangerous live warning classes and disconnected retired allocator implementations from the active build.
- Kept the GNU Make build as the 0.0.5 reference build system.
- Added a standalone Doxygen 1.9.4 release configuration and expanded API/invariant documentation for the active boot, memory, paging, scheduler, interrupt, terminal, keyboard, shell, ACPI, timekeeping, and diagnostic interfaces.
- Brought the Doxygen build to a warning-free state under the 0.0.5 release configuration.
- Added in-tree Lucyna-palette dark themes for both the generated HTML reference and LaTeX/PDF manual.
- Switched Graphviz documentation output to transparent SVG where supported so generated diagrams fit the dark documentation theme.
- Expanded the repository README with the current architecture, build expectations, diagnostics, documentation workflow, and deliberate 0.0.5 boundaries.

### Qualification status

Before tagging, the 0.0.5 branch passed:

- Stage-2 memory torture testing.
- Repeated same-boot memory torture runs.
- Cooperative scheduler operation.
- Dynamic process creation and long scheduler soak testing.
- 22-task cooperative scheduling under load.
- Multiple cold QEMU boots.
- VSH/keyboard operation after standalone-terminal handoff.
- Warning-free Doxygen 1.9.4 generation.

## Version 0.0.6 — Lucyna (Planned / Subject to Change)

0.0.6 is planned as the first release centered on process lifecycle and a more deliberate interactive identity rather than another broad bring-up pass. The repository and project remain VoyagerOS64; **Lucyna** is the 0.0.6 runtime/UI visual direction.

### Planned process and scheduler work

- Add clean task return/exit semantics instead of panicking when a task returns.
- Remove dead tasks from the cooperative run queue and reclaim scheduler-owned task resources.
- Add blocked/waiting process states and wait semantics suitable for foreground commands.
- Add foreground/background job behavior without requiring `fork()`.
- Keep the cooperative scheduler as the qualified baseline while lifecycle semantics are built and tested.

### Planned VSH and interactive work

- Fix VSH backspace/editing behavior.
- Add useful process-oriented commands such as `ps`, with termination/job-control commands considered once lifecycle support exists.
- Improve terminal ownership/output serialization so background task output does not trample interactive input.
- Apply the Lucyna UI palette and presentation consistently across VSH, boot/runtime status output, warnings, errors, and later framebuffer UI work.

### Planned memory/tooling cleanup

- Audit the remaining Stage-2 paging edge cases, including large-page masks and mappings.
- Continue keeping memory diagnostics as permanent VSH regression tools.
- Consider beginning the Make-to-CMake transition only after the 0.0.6 functional work is stable, while retaining the known-good Make build as a reference during migration.

### Deliberately deferred from the initial 0.0.6 scope

- Unix-style `fork()`/`exec()` and full per-process address-space cloning.
- IRQ/PIT preemptive scheduling until interrupt-frame-aware switching is designed and qualified.
- Qualified per-task FPU/XSAVE state preservation.
- APIC/IOAPIC-primary routing and SMP bring-up.
