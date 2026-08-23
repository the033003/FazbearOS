KERNEL_C_DIR := src/impl/kernel
ARCH_C_DIR := src/impl/x86_64
ARCH_ASM_DIR := src/impl/x86_64

BUILD_DIR := build
DIST_DIR := dist/x86_64
ISO_DIR := targets/x86_64/iso
KERNEL_BIN := $(DIST_DIR)/kernel.bin
KERNEL_ISO := $(DIST_DIR)/kernel.iso

CC := x86_64-elf-gcc
LD := x86_64-elf-ld
AS := nasm

CFLAGS := \
	-ffreestanding \
	-mno-red-zone \
	-fno-stack-protector \
	-fno-pic \
	-fno-pie \
	-Wall \
	-Wextra \
	-I src/intf

LDFLAGS := \
	-n \
	-T targets/x86_64/linker.ld

KERNEL_SOURCES := $(shell find $(KERNEL_C_DIR) -name '*.c')
ARCH_C_SOURCES := $(shell find $(ARCH_C_DIR) -name '*.c')
ARCH_ASM_SOURCES := $(shell find $(ARCH_ASM_DIR) -name '*.asm')

KERNEL_OBJECTS := $(patsubst $(KERNEL_C_DIR)/%.c,$(BUILD_DIR)/kernel/%.o,$(KERNEL_SOURCES))
ARCH_C_OBJECTS := $(patsubst $(ARCH_C_DIR)/%.c,$(BUILD_DIR)/x86_64/%.o,$(ARCH_C_SOURCES))
ARCH_ASM_OBJECTS := $(patsubst $(ARCH_ASM_DIR)/%.asm,$(BUILD_DIR)/x86_64/%.o,$(ARCH_ASM_SOURCES))

OBJECTS := \
	$(KERNEL_OBJECTS) \
	$(ARCH_C_OBJECTS) \
	$(ARCH_ASM_OBJECTS)

.PHONY: all
all: build-x86_64

.PHONY: build-x86_64
build-x86_64: $(KERNEL_ISO)

$(KERNEL_ISO): $(KERNEL_BIN)
	mkdir -p $(DIST_DIR)
	cp $(KERNEL_BIN) $(ISO_DIR)/boot/kernel.bin
	grub-mkrescue -o $(KERNEL_ISO) $(ISO_DIR)

$(KERNEL_BIN): $(OBJECTS)
	mkdir -p $(DIST_DIR)
	$(LD) $(LDFLAGS) -o $@ $(OBJECTS)

$(BUILD_DIR)/kernel/%.o: $(KERNEL_C_DIR)/%.c
	mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/x86_64/%.o: $(ARCH_C_DIR)/%.c
	mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/x86_64/%.o: $(ARCH_ASM_DIR)/%.asm
	mkdir -p $(dir $@)
	$(AS) -f elf64 $< -o $@

.PHONY: clean
clean:
	rm -rf $(BUILD_DIR) $(DIST_DIR)

.PHONY: rebuild
rebuild: clean build-x86_64

.PHONY: run
run: build-x86_64
	qemu-system-x86_64 -cdrom $(KERNEL_ISO)

.PHONY: run-debug
run-debug: build-x86_64
	qemu-system-x86_64 \
		-cdrom $(KERNEL_ISO) \
		-no-reboot \
		-no-shutdown
