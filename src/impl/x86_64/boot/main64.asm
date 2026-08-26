BITS 64

global main64

extern kernel_main

section .text

main64:

    ; main.asm has already placed the Multiboot2 information pointer
    ; into RDI, which is the first argument register under the
    ; System V x86_64 calling convention.

    ; Keep interrupts disabled until the kernel installs an IDT.
    cli

    ; Enter the C kernel.
    call kernel_main

.halt:
    cli
    hlt
    jmp .halt
