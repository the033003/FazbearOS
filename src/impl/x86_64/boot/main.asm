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

    ; Clear L4
    mov edi, page_table_l4
    xor eax, eax
    mov ecx, 4096 / 4
    rep stosd

    ; Clear L3
    mov edi, page_table_l3
    xor eax, eax
    mov ecx, 4096 / 4
    rep stosd

    ; Clear all four L2 tables
    mov edi, page_table_l2_0
    xor eax, eax
    mov ecx, 4096 / 4
    rep stosd

    mov edi, page_table_l2_1
    xor eax, eax
    mov ecx, 4096 / 4
    rep stosd

    mov edi, page_table_l2_2
    xor eax, eax
    mov ecx, 4096 / 4
    rep stosd

    mov edi, page_table_l2_3
    xor eax, eax
    mov ecx, 4096 / 4
    rep stosd


    ; L4[0] -> L3
    mov eax, page_table_l3
    or eax, 0x03
    mov [page_table_l4], eax


    ; L3[0] -> first 1 GiB
    mov eax, page_table_l2_0
    or eax, 0x03
    mov [page_table_l3 + 0], eax

    ; L3[1] -> second 1 GiB
    mov eax, page_table_l2_1
    or eax, 0x03
    mov [page_table_l3 + 8], eax

    ; L3[2] -> third 1 GiB
    mov eax, page_table_l2_2
    or eax, 0x03
    mov [page_table_l3 + 16], eax

    ; L3[3] -> fourth 1 GiB
    mov eax, page_table_l2_3
    or eax, 0x03
    mov [page_table_l3 + 24], eax


    ; ----------------------------------------
    ; L2 #0
    ; Maps physical 0x00000000 - 0x3FFFFFFF
    ; ----------------------------------------

    mov edi, page_table_l2_0
    mov eax, 0x00000083
    mov ecx, 512

.map_l2_0:
    mov [edi], eax
    add eax, 0x200000
    add edi, 8
    loop .map_l2_0


    ; ----------------------------------------
    ; L2 #1
    ; Maps physical 0x40000000 - 0x7FFFFFFF
    ; ----------------------------------------

    mov edi, page_table_l2_1
    mov eax, 0x40000083
    mov ecx, 512

.map_l2_1:
    mov [edi], eax
    add eax, 0x200000
    add edi, 8
    loop .map_l2_1


    ; ----------------------------------------
    ; L2 #2
    ; Maps physical 0x80000000 - 0xBFFFFFFF
    ; ----------------------------------------

    mov edi, page_table_l2_2
    mov eax, 0x80000083
    mov ecx, 512

.map_l2_2:
    mov [edi], eax
    add eax, 0x200000
    add edi, 8
    loop .map_l2_2


    ; ----------------------------------------
    ; L2 #3
    ; Maps physical 0xC0000000 - 0xFFFFFFFF
    ; ----------------------------------------

    mov edi, page_table_l2_3
    mov eax, 0xC0000083
    mov ecx, 512

.map_l2_3:
    mov [edi], eax
    add eax, 0x200000
    add edi, 8
    loop .map_l2_3

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

page_table_l2_0:
    resb 4096

page_table_l2_1:
    resb 4096

page_table_l2_2:
    resb 4096

page_table_l2_3:
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
