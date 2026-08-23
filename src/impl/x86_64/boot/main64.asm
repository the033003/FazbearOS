BITS 64

global main64

extern kernel_main
extern multiboot_information

section .text

main64:
    mov rsp, stack_top64

    mov rdi, [multiboot_information]

    call kernel_main

.hang:
    cli
    hlt
    jmp .hang


section .bss

align 16

stack_bottom64:
    resb 16384

stack_top64:
