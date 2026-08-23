BITS 64

global main64

extern kernel_main

section .text

main64:
    mov rsp, stack_top

    mov rdi, [multiboot_information]

    call kernel_main

.hang:
    cli
    hlt
    jmp .hang

section .bss

align 16

stack_bottom:
    resb 16384

stack_top:

section .data

align 8

multiboot_information:
    dq 0
