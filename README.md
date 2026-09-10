# VoyagerOS64

**To Boldly Go.**

VoyagerOS64 is a hobby x86-64 operating-system kernel written as a hands-on systems-programming project. The name is a nod to both the Voyager probes and *Star Trek*, and to this project's original goal: exploring the part of computing that sits below applications, libraries, and hosted runtimes.

## Current development state

The active development line for the 0.0.5 release is `memory_retrofit_stage_2`.

0.0.5 is primarily a memory-management and scheduler-stabilization release. Its current architecture is deliberately small and conservative:

```text
Limine
  -> early_alloc
  -> paging_bootstrap
  -> neo_framealloc
  -> kmalloc
  -> scheduler / drivers / VSH
```

The Stage-2 memory stack has been runtime-qualified with direct PMM tests, heap boundary tests, fragmentation/coalescing tests, `krealloc` preservation tests, multi-megabyte heap growth, deterministic randomized allocation torture, and repeated same-boot test cycles.

The scheduler is currently **cooperative round-robin**. Tasks voluntarily yield with `schedule()`. IRQ/PIT-driven preemption is intentionally disabled for 0.0.5 until context switching is redesigned around a complete interrupt-frame model.

## Notable 0.0.5 components

- Limine boot protocol and higher-half kernel entry
- Voyager-owned page tables and CR3 handoff
- early physical-page bump allocator for bootstrap
- neo physical frame allocator
- kernel heap (`kmalloc`, `kfree`, `krealloc`)
- cooperative process scheduler
- PIC/PIT and IDT/GDT bring-up
- PS/2 keyboard queue
- framebuffer terminal
- VSH interactive shell
- built-in `memtest` memory qualification command
- dynamic scheduler soak-task command

## Important architectural rules

Physical and virtual addresses are intentionally kept distinct:

- the PMM owns **physical frames**;
- paging assigns **virtual mappings** to those frames;
- the HHDM is used when the kernel needs a virtual address for physical memory;
- `kmalloc` returns kernel virtual addresses and is the general-purpose allocator after memory bring-up.

The active allocator path is singular. Older allocator, VMM, stream/tube, and C++ allocation experiments remain in the repository for historical reference but are explicitly excluded from the active kernel link by `GNUmakefile`.

## Building

The build expects an x86-64 ELF cross-toolchain under:

```text
$HOME/opt/cross/bin/x86_64-elf-gcc
$HOME/opt/cross/bin/x86_64-elf-ld
```

It also requires NASM, xorriso, and the Limine files expected by the build scripts.

Typical build:

```sh
make clean
make
```

The resulting bootable image is:

```text
VoyagerOS.iso
```

The Makefile is currently the reference build system. A CMake migration is intentionally postponed until after the 0.0.5 stabilization release.

## VSH diagnostics

Useful built-in commands currently include:

```text
time      print the current system time/date information
memtest   run the Stage-2 memory qualification suite
soak      spawn another cooperative scheduler soak task
halt      halt the system
```

`memtest` is intentionally retained as a permanent kernel diagnostic rather than being removed after release qualification.

## 0.0.5 design boundaries

The following are intentionally not claimed as finished in 0.0.5:

- preemptive scheduling
- task exit/reaping and wait semantics
- Unix-style `fork()`/`exec()` process creation
- per-process virtual address spaces
- scheduler-integrated FPU/XSAVE state
- APIC-based interrupt routing as the primary path
- SMP scheduling

Those belong to later development rather than being quietly half-enabled in the 0.0.5 release.

## Documentation

The source tree is documented with Doxygen-style comments. The documentation is organized around boot, memory, paging, interrupts, scheduling, drivers, shell services, diagnostics, and retired/historical subsystems.

Generate the 0.0.5 HTML and LaTeX reference trees with:

```sh
doxygen Doxygen-0.0.5
```

The generated HTML reference is written to `html/`. The 0.0.5 documentation configuration applies the repository-owned low-glare Voyager dark theme in `docs/theme/voyager-dark.css`.

Build the dark PDF reference manual with:

```sh
cd latex
make
```

The final manual is `latex/refman.pdf`. Its dark page, text, link, and syntax palette is supplied by `docs/theme/voyager-dark.sty` through Doxygen's `LATEX_EXTRA_STYLESHEET` mechanism. Generated `html/` and `latex/` files should not be edited by hand; change the source comments or theme files and regenerate them instead.

## License

VoyagerOS64 is released under the MIT License. See `LICENSE.md`.
