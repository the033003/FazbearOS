BITS 32

global start
global long_mode_start
global multiboot_information

extern main64

section .text

start:
    cli

    mov esp, stack_top

    cmp eax, 0x36d76289
    jne .no_multiboot

    mov [multiboot_magic], eax
    mov [multiboot_information], ebx

    call setup_page_tables

    lgdt [gdt64_pointer]

    mov eax, cr4
    or eax, 1 << 5
    mov cr4, eax

    mov eax, page_table_l4
    mov cr3, eax

    mov ecx, 0xC0000080
    rdmsr
    or eax, 1 << 8
    wrmsr

    mov eax, cr0
    or eax, 1 << 31
    mov cr0, eax

    jmp 0x08:long_mode_start

.no_multiboot:
    hlt
    jmp .no_multiboot


setup_page_tables:
    mov edi, page_table_l4
    xor eax, eax
    mov ecx, 4096
    rep stosd

    mov eax, page_table_l3
    or eax, 0x03
    mov [page_table_l4], eax

    mov eax, page_table_l2
    or eax, 0x03
    mov [page_table_l3], eax

    mov edi, page_table_l2
    mov eax, 0x00000083
    mov ecx, 512

.map:
    mov [edi], eax
    add eax, 0x200000
    add edi, 8
    loop .map

    ret


BITS 64

long_mode_start:
    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov ss, ax

    mov rsp, stack_top64

    mov rdi, [multiboot_information]

    call main64

.hang:
    cli
    hlt
    jmp .hang


section .rodata

gdt64:
    dq 0
    dq 0x00AF9A000000FFFF
    dq 0x00AF92000000FFFF

gdt64_pointer:
    dw gdt64_pointer - gdt64 - 1
    dq gdt64


section .bss

align 4096

page_table_l4:
    resb 4096

page_table_l3:
    resb 4096

page_table_l2:
    resb 4096

align 16

stack_bottom:
    resb 16384

stack_top:

align 16

stack_bottom64:
    resb 16384

stack_top64:

align 8

multiboot_magic:
    resq 1

multiboot_information:
    resq 1
