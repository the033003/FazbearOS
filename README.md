# FazbearOS

A hobby x86_64 operating system written in C and Assembly.

FazbearOS currently boots through Multiboot2 + GRUB into a custom graphical desktop with working PS/2 mouse and keyboard input, draggable windows, a taskbar, Start menu, and the Nibble scratchpad application.

---
<img width="1026" height="768" alt="image" src="https://github.com/user-attachments/assets/d4cebb9e-19a6-4705-b1b8-a7c6e23b8ff1" />
---

## Features

### Boot & Kernel

- Multiboot2 + GRUB boot
- x86_64 long mode
- 64-bit GDT setup
- Identity-mapped paging for the first 4 GiB
- Physical memory manager
- Kernel heap
- Interrupt Descriptor Table (IDT)
- Programmable Interrupt Controller (PIC)
- RTC support
- Kernel logging
- Basic kernel shell

### Graphics & Desktop

- Linear framebuffer graphics
- 1024×768 framebuffer support
- Custom software-rendered desktop
- Top desktop bar
- Taskbar
- Start menu
- Desktop grid/background
- Window borders and title bars
- Active/inactive window states
- Window focus management
- Draggable windows
- Window minimize/close controls
- Taskbar window buttons
- Custom software mouse cursor

### Input

- PS/2 keyboard driver
- PS/2 mouse driver
- Mouse interrupt handling
- Mouse packet synchronization
- Mouse movement and button tracking
- Screen-bound mouse coordinates
- Left, right, and middle mouse button support
- Smooth window dragging

### Applications

- Nibble scratchpad application
- Launch Nibble from the Start menu
- Minimize and restore applications from the taskbar
- Close and relaunch applications
- Keyboard input while Nibble is focused

### Storage & Filesystem

- Physical memory management
- Kernel heap
- Basic VFS
- RAM filesystem (ramfs)

## Running a prebuilt ISO

1. Download the latest ISO from the **Releases** tab.
2. Run it with QEMU:

```bash
qemu-system-x86_64 \
 -cdrom dist/x86_64/kernel.iso \
 -m 256M \
 -machine type=pc,accel=tcg \
 -device nec-usb-xhci \
 -device usb-tablet \
 -display sdl
```
