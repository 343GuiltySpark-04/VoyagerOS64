# This is the name that our final kernel executable will have.
# Change as needed.
override KERNEL := warp.bin
 
# Convenience macro to reliably declare overridable command variables.
define DEFAULT_VAR =
    ifeq ($(origin $1),default)
        override $(1) := $(2)
    endif
    ifeq ($(origin $1),undefined)
        override $(1) := $(2)
    endif
endef
 
# It is highly recommended to use a custom built cross toolchain to build a kernel.
export CC=$(HOME)/opt/cross/bin/x86_64-elf-gcc
export LD=$(HOME)/opt/cross/bin/x86_64-elf-ld
 
# User controllable CFLAGS.
CFLAGS ?= -g -O1 -pipe -Wall -Wextra -finline-functions
 
# User controllable preprocessor flags. We set none by default.
CPPFLAGS ?= 
 
# User controllable nasm flags.
NASMFLAGS ?= -O1 -F dwarf -g
 
# User controllable linker flags. We set none by default.
LDFLAGS ?=
 
# Internal C flags that should not be changed by the user.
override CFLAGS +=       \
    -std=gnu11          \
    -ffreestanding      \
    -fno-stack-protector \
    -fno-stack-check    \
    -fno-lto            \
    -fno-pie            \
    -fno-pic            \
    -m64                 \
    -march=x86-64        \
    -mabi=sysv           \
    -mno-mmx             \
    -mno-sse2            \
    -mno-red-zone        \
    -mcmodel=kernel      \
    -fms-extensions      \
    -MMD                 \
    -I.
 
# Internal linker flags that should not be changed by the user.
override LDFLAGS +=         \
    -nostdlib               \
    -static                 \
    -m elf_x86_64           \
    -z max-page-size=0x1000 \
    -z noexecstack          \
    -T linker.ld
 
# Check if the linker supports -no-pie and enable it if it does.
ifeq ($(shell $(LD) --help 2>&1 | grep 'no-pie' >/dev/null 2>&1; echo $$?),0)
    override LDFLAGS += -no-pie
endif
 
# Internal nasm flags that should not be changed by the user.
override NASMFLAGS += \
    -f elf64
 
# Stage-2 keeps old implementations in-tree for history/reference, but they are
# no longer linked into the active kernel. This prevents stale allocators and
# the abandoned tube I/O layer from silently becoming live dependencies.
override LEGACY_CFILES := \
    src/heap.c \
    src/bucket.c \
    src/pebble.c \
    src/frameallocator.c \
    src/vmm.c \
    src/streams.c \
    src/liballoc.c \
    src/memUtils.c \
    src/mm/liballoc_glue.c

override LEGACY_CCFILES := \
    src/alloc.cpp \
    src/lock.cpp \
    src/lock_atm_c.cpp

# Use find for source discovery, then explicitly prune retired implementations.
override ALL_CFILES := $(shell find src -type f -name '*.c')
override ALL_CCFILES := $(shell find src -type f -name '*.cpp')
override CFILES := $(filter-out $(LEGACY_CFILES),$(ALL_CFILES))
override CCFILES := $(filter-out $(LEGACY_CCFILES),$(ALL_CCFILES))
override ASFILES := $(shell find src -type f -name '*.S')
override NASMFILES := $(shell find src -type f -name '*.asm')
override OBJ := $(CFILES:.c=.o) $(ASFILES:.S=.o) $(NASMFILES:.asm=.o) $(CCFILES:.cpp=.o)
override HEADER_DEPS := $(CFILES:.c=.d) $(ASFILES:.S=.d) $(CCFILES:.cpp=.d)
 
# Default target.
.PHONY: all
all: $(KERNEL) iso
 
# Link rules for the final kernel executable.
$(KERNEL): $(OBJ)
	$(LD) $(OBJ) $(LDFLAGS) -o $@
 
# Include header dependencies.
-include $(HEADER_DEPS)
 
# Compilation rules for *.c files.
%.o: %.c
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

%.o: %.cpp
	gcc $(CPPFLAGS) $(filter-out -std=gnu11,$(CFLAGS)) -std=gnu++17 -c $< -o $@
 
# Compilation rules for *.S files.
%.o: %.S
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@
 
# Compilation rules for *.asm (nasm) files.
%.o: %.asm
	nasm $(NASMFLAGS) $< -o $@
 
# Remove object files and the final executable.
.PHONY: clean
clean:
	rm -rvf $(KERNEL) $(OBJ) $(HEADER_DEPS)
	rm -rvf ./iso_root
	rm -rfv *.iso
	rm -rfv *.log

iso:
	sh buildlimine.sh
	mkdir -pv iso_root
	cp -v warp.bin limine.cfg limine/limine.sys \
      limine/limine-cd.bin limine/limine-cd-efi.bin iso_root/
	xorriso -as mkisofs -b limine-cd.bin \
        -no-emul-boot -boot-load-size 4 -boot-info-table \
        --efi-boot limine-cd-efi.bin \
        -efi-boot-part --efi-boot-image --protective-msdos-label \
        iso_root -o VoyagerOS.iso
	limine/limine-deploy VoyagerOS.iso

.PHONY: qemu-linux
qemu-linux:
	sh qemu.sh
