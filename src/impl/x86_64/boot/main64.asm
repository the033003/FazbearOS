BITS 64

global main64

extern kernel_main
extern multiboot_information

section .text

main64:
    ; main.asm has already established a valid 64-bit stack.
    ; Do not replace or redefine that stack here.

    ; System V AMD64 ABI:
    ; RSP must be 16-byte aligned immediately before CALL.
    sub rsp, 8

    ; Pass the Multiboot2 information pointer as the first
    ; argument to kernel_main().
    mov edi, dword [multiboot_information]

    ; Keep interrupts disabled until the kernel installs
    ; a valid IDT.
    cli
    cld

    call kernel_main

.hang:
    cli
    hlt
    jmp .hang
