global long_mode_start
extern kernel_main

section .text
bits 64

long_mode_start:
    ; Load the 64-bit data segment.
    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov fs, ax
    mov gs, ax

    ; Use the stack owned by the 64-bit entry stage.
    mov rsp, stack_top
    and rsp, -16

    ; C code expects the direction flag to be clear.
    cld

    ; GRUB's Multiboot2 registers survive the transition into long mode:
    ;
    ; EAX = Multiboot2 magic
    ; EBX = physical address of Multiboot2 information structure
    ;
    ; Pass them to kernel_main as:
    ;
    ; RDI = multiboot information pointer
    ; RSI = multiboot magic
    ;
    mov edi, ebx
    mov esi, eax

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
