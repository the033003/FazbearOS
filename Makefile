TARGET := x86_64

CC := x86_64-elf-gcc
LD := x86_64-elf-ld
AS := nasm

CFLAGS := \
	-std=gnu11 \
	-ffreestanding \
	-fno-builtin \
	-fno-stack-protector \
	-fno-pic \
	-fno-pie \
	-fno-asynchronous-unwind-tables \
	-fno-unwind-tables \
	-fno-exceptions \
	-mno-red-zone \
	-mno-mmx \
	-mno-sse \
	-mno-sse2 \
	-mgeneral-regs-only \
	-O2 \
	-Wall \
	-Wextra \
	-Werror \
	-I src/intf

LDFLAGS := \
	-n \
	-z max-page-size=0x1000 \
	-T targets/x86_64/linker.ld

BUILD_DIR := build
DIST_DIR := dist/x86_64
ISO_DIR := targets/x86_64/iso

KERNEL_BIN := $(DIST_DIR)/kernel.bin
KERNEL_ISO := $(DIST_DIR)/kernel.iso

C_SOURCES := $(shell find src -type f -name '*.c')
ASM_SOURCES := $(shell find src -type f -name '*.asm')

C_OBJECTS := $(patsubst src/%.c,$(BUILD_DIR)/%.o,$(C_SOURCES))
ASM_OBJECTS := $(patsubst src/%.asm,$(BUILD_DIR)/%.o,$(ASM_SOURCES))

OBJECTS := $(C_OBJECTS) $(ASM_OBJECTS)

.PHONY: all build-x86_64 clean run

all: build-x86_64

build-x86_64: $(KERNEL_ISO)

$(KERNEL_BIN): $(OBJECTS)
	mkdir -p $(DIST_DIR)

	$(LD) $(LDFLAGS) \
		-Map=$(DIST_DIR)/kernel.map \
		-o $@ \
		$(OBJECTS)

$(KERNEL_ISO): $(KERNEL_BIN)
	mkdir -p $(ISO_DIR)/boot

	cp $(KERNEL_BIN) \
		$(ISO_DIR)/boot/kernel.bin

	grub-file \
		--is-x86-multiboot2 \
		$(KERNEL_BIN)

	grub-mkrescue \
		-o $@ \
		$(ISO_DIR)

$(BUILD_DIR)/%.o: src/%.c
	mkdir -p $(dir $@)

	$(CC) $(CFLAGS) \
		-c $< \
		-o $@

$(BUILD_DIR)/%.o: src/%.asm
	mkdir -p $(dir $@)

	$(AS) -f elf64 \
		$< \
		-o $@

clean:
	rm -rf $(BUILD_DIR)
	rm -rf $(DIST_DIR)

	rm -f \
		targets/x86_64/iso/boot/kernel.bin

run: $(KERNEL_ISO)
	qemu-system-x86_64 \
		-cdrom $(KERNEL_ISO) \
		-m 256M
