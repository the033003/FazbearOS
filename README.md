# FazbearOS

Please don't sue me Scott.

A hobby x86_64 operating system written in C and Assembly.  
It boots into a simple graphical desktop with a working mouse and keyboard.

## Features

- Multiboot2 + GRUB boot
- 64-bit long mode
- Physical memory manager + heap
- Interrupt handling (IDT + PIC)
- PS/2 keyboard and mouse
- Linear framebuffer graphics
- Basic windowed desktop (draggable window, taskbar, cursor)
- Simple VFS + ramfs
- Logging and a basic shell

## Running a prebuilt ISO

1. Download the latest ISO from the **Releases** tab.
2. Run it with QEMU:

```bash
qemu-system-x86_64 -cdrom path/to/kernel.iso -m 256M
```

Click inside the QEMU window to capture the mouse.  
Press `Left Ctrl + Left Alt` to release it.

You can also burn the ISO to a USB stick and boot it on real hardware.

> VirtualBox support is untested / unreliable. QEMU is recommended.

## Building from source

**Recommended:** Linux + Docker.

### Requirements

- Docker (daemon running)
- QEMU (`qemu-system-x86_64`)
- (Optional) a text editor

### Steps

```bash
# 1. Build the toolchain container
sudo docker build buildenv -t fazbear-buildenv

# 2. Enter the container (mounts the project)
sudo docker run --rm -it -v "$(pwd)":/root/env fazbear-buildenv

# 3. Inside the container – build the ISO
make build-x86_64

# 4. Exit the container
exit

# 5. Run it
qemu-system-x86_64 -cdrom dist/x86_64/kernel.iso -m 256M
```

### Useful Make targets

| Command              | Description                  |
|----------------------|------------------------------|
| `make build-x86_64`  | Build kernel + ISO           |
| `make clean`         | Remove build artifacts       |
| `make run`           | Build and launch in QEMU     |

### Cleaning up Docker

```bash
docker rmi fazbear-buildenv -f
```

## Project layout

```
src/
├── impl/
│   ├── kernel/          # Core kernel, desktop, memory, VFS…
│   └── x86_64/          # Architecture-specific (boot, interrupts, drivers)
└── intf/                # Public headers
targets/x86_64/          # Linker script + ISO structure
buildenv/                # Docker toolchain
```

## License

GPL-3.0

---
