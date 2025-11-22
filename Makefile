# NovaeOS Makefile
# Main build system for the operating system

# Toolchain configuration
AS := nasm
CC := gcc
LD := ld
AR := ar

# Flags
ASFLAGS := -f elf64
CFLAGS := -std=c11 -ffreestanding -O2 -Wall -Wextra -fno-exceptions \
          -nostdlib -nostdinc -fno-builtin -fno-stack-protector \
          -mno-red-zone -mcmodel=kernel -mno-mmx -mno-sse -mno-sse2 \
          -Ikernel/include
LDFLAGS := -nostdlib -n -T linker.ld

# Directories
BUILD_DIR := build
ISO_DIR := iso
KERNEL_DIR := kernel
BOOT_DIR := boot

# Output files
KERNEL_ELF := $(BUILD_DIR)/kernel.elf
ISO_FILE := $(BUILD_DIR)/novae.iso

# Source files
ASM_SOURCES := $(shell find $(KERNEL_DIR) -name '*.S')
C_SOURCES := $(shell find $(KERNEL_DIR) -name '*.c')

# Object files
ASM_OBJECTS := $(patsubst %.S,$(BUILD_DIR)/%.o,$(ASM_SOURCES))
C_OBJECTS := $(patsubst %.c,$(BUILD_DIR)/%.o,$(C_SOURCES))
ALL_OBJECTS := $(ASM_OBJECTS) $(C_OBJECTS)

# QEMU configuration
QEMU := qemu-system-x86_64
QEMU_FLAGS := -m 512M -serial stdio
QEMU_DEBUG_FLAGS := -s -S

# Colors for output
COLOR_RESET := \033[0m
COLOR_GREEN := \033[32m
COLOR_BLUE := \033[34m
COLOR_YELLOW := \033[33m

# Default target
.PHONY: all
all: $(KERNEL_ELF)
	@echo "$(COLOR_GREEN)Build complete!$(COLOR_RESET)"
	@echo "Run 'make iso' to create bootable ISO"
	@echo "Run 'make run' to test in QEMU"

# Build kernel ELF
$(KERNEL_ELF): $(ALL_OBJECTS) linker.ld
	@echo "$(COLOR_BLUE)Linking kernel...$(COLOR_RESET)"
	@mkdir -p $(BUILD_DIR)
	$(LD) $(LDFLAGS) -o $@ $(ALL_OBJECTS)
	@echo "$(COLOR_GREEN)Kernel linked: $@$(COLOR_RESET)"

# Compile assembly files
$(BUILD_DIR)/%.o: %.S
	@echo "$(COLOR_YELLOW)Assembling $<...$(COLOR_RESET)"
	@mkdir -p $(dir $@)
	$(AS) $(ASFLAGS) $< -o $@

# Compile C files
$(BUILD_DIR)/%.o: %.c
	@echo "$(COLOR_YELLOW)Compiling $<...$(COLOR_RESET)"
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

# Create bootable ISO
.PHONY: iso
iso: $(KERNEL_ELF)
	@echo "$(COLOR_BLUE)Creating bootable ISO...$(COLOR_RESET)"
	@mkdir -p $(ISO_DIR)/boot/grub
	@cp $(KERNEL_ELF) $(ISO_DIR)/boot/kernel.elf
	@cp $(BOOT_DIR)/grub.cfg $(ISO_DIR)/boot/grub/grub.cfg
	@grub-mkrescue -o $(ISO_FILE) $(ISO_DIR) 2>/dev/null || \
		(echo "$(COLOR_YELLOW)Warning: grub-mkrescue not found or failed$(COLOR_RESET)"; \
		 echo "$(COLOR_YELLOW)Please install: sudo apt install grub-pc-bin grub-common xorriso$(COLOR_RESET)")
	@if [ -f $(ISO_FILE) ]; then \
		echo "$(COLOR_GREEN)ISO created: $(ISO_FILE)$(COLOR_RESET)"; \
	fi

# Run in QEMU
.PHONY: run
run: iso
	@echo "$(COLOR_BLUE)Starting QEMU...$(COLOR_RESET)"
	@if [ ! -f $(ISO_FILE) ]; then \
		echo "$(COLOR_YELLOW)ISO file not found, please run 'make iso' first$(COLOR_RESET)"; \
		exit 1; \
	fi
	$(QEMU) -cdrom $(ISO_FILE) $(QEMU_FLAGS)

# Run in QEMU with GDB server
.PHONY: debug
debug: iso
	@echo "$(COLOR_BLUE)Starting QEMU with GDB server...$(COLOR_RESET)"
	@echo "$(COLOR_YELLOW)Connect with: gdb $(KERNEL_ELF) -ex 'target remote localhost:1234'$(COLOR_RESET)"
	$(QEMU) -cdrom $(ISO_FILE) $(QEMU_FLAGS) $(QEMU_DEBUG_FLAGS)

# Run with network enabled
.PHONY: run-net
run-net: iso
	@echo "$(COLOR_BLUE)Starting QEMU with networking...$(COLOR_RESET)"
	$(QEMU) -cdrom $(ISO_FILE) $(QEMU_FLAGS) \
		-netdev user,id=net0 \
		-device e1000,netdev=net0

# Run with disk image
.PHONY: run-disk
run-disk: iso disk.img
	@echo "$(COLOR_BLUE)Starting QEMU with disk...$(COLOR_RESET)"
	$(QEMU) -cdrom $(ISO_FILE) $(QEMU_FLAGS) \
		-drive file=disk.img,format=raw,index=0,media=disk

# Create disk image
disk.img:
	@echo "$(COLOR_BLUE)Creating disk image...$(COLOR_RESET)"
	qemu-img create -f raw disk.img 100M
	@echo "$(COLOR_GREEN)Disk image created: disk.img$(COLOR_RESET)"

# Clean build artifacts
.PHONY: clean
clean:
	@echo "$(COLOR_BLUE)Cleaning build artifacts...$(COLOR_RESET)"
	rm -rf $(BUILD_DIR) $(ISO_DIR)
	@echo "$(COLOR_GREEN)Clean complete!$(COLOR_RESET)"

# Show help
.PHONY: help
help:
	@echo "NovaeOS Build System"
	@echo ""
	@echo "Targets:"
	@echo "  all        - Build kernel (default)"
	@echo "  iso        - Create bootable ISO image"
	@echo "  run        - Run in QEMU"
	@echo "  debug      - Run in QEMU with GDB server (port 1234)"
	@echo "  run-net    - Run with networking enabled"
	@echo "  run-disk   - Run with disk image"
	@echo "  clean      - Remove build artifacts"
	@echo "  help       - Show this help message"
	@echo ""
	@echo "Debug workflow:"
	@echo "  1. Terminal 1: make debug"
	@echo "  2. Terminal 2: gdb $(KERNEL_ELF)"
	@echo "                 (gdb) target remote localhost:1234"
	@echo "                 (gdb) break kernel_main"
	@echo "                 (gdb) continue"

# Show current configuration
.PHONY: info
info:
	@echo "Build Configuration:"
	@echo "  AS       = $(AS)"
	@echo "  CC       = $(CC)"
	@echo "  LD       = $(LD)"
	@echo "  ASFLAGS  = $(ASFLAGS)"
	@echo "  CFLAGS   = $(CFLAGS)"
	@echo "  LDFLAGS  = $(LDFLAGS)"
	@echo ""
	@echo "Files:"
	@echo "  ASM Sources: $(words $(ASM_SOURCES))"
	@echo "  C Sources:   $(words $(C_SOURCES))"
	@echo "  Objects:     $(words $(ALL_OBJECTS))"
	@echo ""
	@echo "Toolchain check:"
	@which $(AS) > /dev/null && echo "  nasm:    OK" || echo "  nasm:    NOT FOUND"
	@which $(CC) > /dev/null && echo "  gcc:     OK" || echo "  gcc:     NOT FOUND"
	@which $(LD) > /dev/null && echo "  ld:      OK" || echo "  ld:      NOT FOUND"
	@which grub-mkrescue > /dev/null && echo "  grub:    OK" || echo "  grub:    NOT FOUND"
	@which $(QEMU) > /dev/null && echo "  qemu:    OK" || echo "  qemu:    NOT FOUND"

.PHONY: check-tools
check-tools:
	@echo "Checking required tools..."
	@which $(AS) > /dev/null || (echo "ERROR: nasm not found"; exit 1)
	@which $(CC) > /dev/null || (echo "ERROR: gcc not found"; exit 1)
	@which $(LD) > /dev/null || (echo "ERROR: ld not found"; exit 1)
	@which grub-mkrescue > /dev/null || (echo "WARNING: grub-mkrescue not found")
	@which $(QEMU) > /dev/null || (echo "ERROR: qemu-system-x86_64 not found"; exit 1)
	@echo "All required tools found!"
