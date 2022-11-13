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
# We are only using "cc" as a placeholder here. It may work by using
# the host system's toolchain, but this is not guaranteed.
export CC=$(HOME)/opt/cross/bin/x86_64-elf-gcc
 
# Same thing for "ld" (the linker).
export LD=$(HOME)/opt/cross/bin/x86_64-elf-ld
 
# User controllable CFLAGS.
CFLAGS ?= -g -O2 -pipe -Wall -Wextra
 
# User controllable preprocessor flags. We set none by default.
CPPFLAGS ?= 
 
# User controllable nasm flags.
NASMFLAGS ?= -O2 -F dwarf -g
 
# User controllable linker flags. We set none by default.
LDFLAGS ?=
 
# Internal C flags that should not be changed by the user.
override CFLAGS +=       \
    -std=gnu11             \
    -ffreestanding       \
    -fno-stack-protector \
    -fno-stack-check     \
    -fno-lto             \
    -fno-pie             \
    -fno-pic             \
    -m64                 \
    -march=native        \
    -mabi=sysv           \
    -mno-80387           \
    -mno-mmx             \
    -mno-sse2            \
    -mno-red-zone        \
    -mcmodel=kernel      \
    -fms-extensions      \
    -Xassembler -mintel64 \
    -MMD                 \
	-DPRINTF_DISABLE_SUPPORT_FLOAT \
    -I.
 
# Internal linker flags that should not be changed by the user.
override LDFLAGS +=         \
    -nostdlib               \
    -static                 \
    -m elf_x86_64           \
    -z max-page-size=0x1000 \
    -T linker.ld
 
# Check if the linker supports -no-pie and enable it if it does.
ifeq ($(shell $(LD) --help 2>&1 | grep 'no-pie' >/dev/null 2>&1; echo $$?),0)
    override LDFLAGS += -no-pie
endif
 
# Internal nasm flags that should not be changed by the user.
override NASMFLAGS += \
    -f elf64
 
# Use find to glob all *.c, *.S, and *.asm files in the directory and extract the object names.
override CFILES := $(shell find src -type f -name '*.c')
override CCFILES := $(shell find src -type f -name '*.cpp')
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
	gcc $(CPPFLAGS) $(CFLAGS) -c $< -o $@
 
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
	mkdir -v iso_root
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