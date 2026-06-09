TARGET     := kernel.elf
ISO        := kyronix.iso
LIMINE_DIR := limine-binary
SRC_DIR    := kernel

DEBUG ?= 0

ifeq ($(DEBUG), 0)
OPT       := -O2
BUILD_DIR := build
else
OPT       := -O0 -g
BUILD_DIR := build-debug
endif

ifneq (, $(shell which x86_64-elf-gcc 2>/dev/null))
    CC := x86_64-elf-gcc
    LD := x86_64-elf-ld
else
    CC := gcc
    LD := ld
endif

CFLAGS := \
    -std=c11           \
    $(OPT)             \
    -Wall -Wextra      \
    -Wno-unused-parameter \
    -ffreestanding     \
    -fno-stack-protector \
    -fno-pic -fno-pie  \
    -m64 -march=x86-64 \
    -mno-80387         \
    -mno-mmx           \
    -mno-sse           \
    -mno-sse2          \
    -mno-red-zone      \
    -mcmodel=kernel    \
    -MMD -MP           \
    -Ikernel           \
    -Ikernel/boot

LDFLAGS := \
    -T linker.ld       \
    -nostdlib          \
    -static            \
    -z max-page-size=0x1000

SRCS    := $(shell find $(SRC_DIR) -name '*.c')
ASM_SRCS:= $(shell find $(SRC_DIR) -name '*.S')
OBJS    := $(patsubst $(SRC_DIR)/%.c,$(BUILD_DIR)/%.o,$(SRCS))
OBJS    += $(patsubst $(SRC_DIR)/%.S,$(BUILD_DIR)/%.o,$(ASM_SRCS))
DEPS    := $(filter %.d,$(OBJS:.o=.d))

QEMU_FLAGS := -M q35 -m 2G -cdrom $(ISO) -boot d -serial stdio -no-reboot -no-shutdown

.PHONY: all iso run run-serial run-uefi debug gdb clean

all: $(BUILD_DIR)/$(TARGET)

$(BUILD_DIR)/$(TARGET): $(OBJS)
	$(LD) $(LDFLAGS) -o $@ $^

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c
	mkdir -p $(@D)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.S
	mkdir -p $(@D)
	$(CC) -std=c11 $(OPT) -ffreestanding -fno-pic -fno-pie -m64 -march=x86-64 \
	      -Ikernel -Ikernel/boot -c $< -o $@

-include $(DEPS)

$(LIMINE_DIR)/limine: $(LIMINE_DIR)/limine.c
	$(MAKE) -C $(LIMINE_DIR)

iso: $(BUILD_DIR)/$(TARGET) $(LIMINE_DIR)/limine
	rm -rf iso_root
	mkdir -p iso_root/boot/limine
	mkdir -p iso_root/EFI/BOOT

	cp $(BUILD_DIR)/$(TARGET) iso_root/boot/kernel.elf
	cp limine.conf            iso_root/boot/limine/limine.conf
	cp $(LIMINE_DIR)/limine-bios.sys    iso_root/boot/limine/
	cp $(LIMINE_DIR)/limine-bios-cd.bin iso_root/boot/limine/
	cp $(LIMINE_DIR)/limine-uefi-cd.bin iso_root/boot/limine/
	cp $(LIMINE_DIR)/BOOTX64.EFI        iso_root/EFI/BOOT/
	cp $(LIMINE_DIR)/BOOTIA32.EFI       iso_root/EFI/BOOT/

	xorriso -as mkisofs              \
	    -b boot/limine/limine-bios-cd.bin \
	    -no-emul-boot                \
	    -boot-load-size 4            \
	    -boot-info-table             \
	    --efi-boot boot/limine/limine-uefi-cd.bin \
	    -efi-boot-part               \
	    --efi-boot-image             \
	    --protective-msdos-label     \
	    iso_root -o $(ISO)

	./$(LIMINE_DIR)/limine bios-install $(ISO)
	@echo ""
	@echo "  Built: $(ISO)"

debug:
	$(MAKE) DEBUG=1 all

gdb:
	$(MAKE) DEBUG=1 iso && \
	qemu-system-x86_64 $(QEMU_FLAGS) -display none -s -S &
	sleep 0.5
	gdb build-debug/kernel.elf -ex "target remote :1234"

run: iso
	qemu-system-x86_64 $(QEMU_FLAGS) \
	    -vga qxl -global qxl-vga.vgamem_mb=1024

run-serial: iso
	qemu-system-x86_64 $(QEMU_FLAGS) \
	    -display none

OVMF ?= /usr/share/edk2/x64/OVMF.fd

run-uefi: iso
	qemu-system-x86_64 $(QEMU_FLAGS) \
	    -bios $(OVMF) \
	    -vga qxl -global qxl-vga.vgamem_mb=1024

clean:
	rm -rf $(BUILD_DIR) $(ISO)
	rm -rf iso_root
	$(MAKE) -C $(LIMINE_DIR) clean
