BITS 32

global start
global long_mode_start
global multiboot_information

extern main64

section .text

start:
    cli

    ; Save the Multiboot2 information pointer immediately.
    ;
    ; GRUB provides:
    ;   EAX = 0x36D76289
    ;   EBX = physical address of the Multiboot2 information.
    cmp eax, 0x36D76289
    jne .halt

    mov [multiboot_magic], eax
    mov [multiboot_information], ebx

    ; Establish a known 32-bit stack before doing anything else.
    mov esp, stack_top

    ; Build identity mappings for the first 4 GiB.
    call setup_page_tables

    ; Load our 64-bit GDT while we are still in protected mode.
    lgdt [gdt64_pointer]

    ; Enable PAE.
    mov eax, cr4
    or eax, 0x20
    mov cr4, eax

    ; Load PML4.
    mov eax, page_table_l4
    mov cr3, eax

    ; Enable Long Mode.
    mov ecx, 0xC0000080
    rdmsr
    or eax, 0x00000100
    wrmsr

    ; Enable paging.
    mov eax, cr0
    or eax, 0x80000000
    mov cr0, eax

    ; Far jump into the 64-bit code segment.
    jmp 0x08:long_mode_start


.halt:
    cli

.halt_loop:
    hlt
    jmp .halt_loop


; ---------------------------------------------------------------------------
; Identity-map the first 4 GiB.
;
; PML4
;   |
;   +-- PDP[0] -> PD[0] -> 0x00000000 - 0x3FFFFFFF
;   +-- PDP[1] -> PD[1] -> 0x40000000 - 0x7FFFFFFF
;   +-- PDP[2] -> PD[2] -> 0x80000000 - 0xBFFFFFFF
;   +-- PDP[3] -> PD[3] -> 0xC0000000 - 0xFFFFFFFF
;
; Each PD contains 512 x 2 MiB pages.
; ---------------------------------------------------------------------------

setup_page_tables:

    ; Clear PML4.
    mov edi, page_table_l4
    xor eax, eax
    mov ecx, 1024
    rep stosd

    ; Clear PDP.
    mov edi, page_table_l3
    xor eax, eax
    mov ecx, 1024
    rep stosd

    ; Clear all four page directories.
    mov edi, page_table_l2_0
    xor eax, eax
    mov ecx, 1024
    rep stosd

    mov edi, page_table_l2_1
    xor eax, eax
    mov ecx, 1024
    rep stosd

    mov edi, page_table_l2_2
    xor eax, eax
    mov ecx, 1024
    rep stosd

    mov edi, page_table_l2_3
    xor eax, eax
    mov ecx, 1024
    rep stosd


    ; PML4[0] -> PDP.
    mov eax, page_table_l3
    or eax, 0x03
    mov [page_table_l4 + 0], eax


    ; PDP[0] -> PD0.
    mov eax, page_table_l2_0
    or eax, 0x03
    mov [page_table_l3 + 0], eax

    ; PDP[1] -> PD1.
    mov eax, page_table_l2_1
    or eax, 0x03
    mov [page_table_l3 + 8], eax

    ; PDP[2] -> PD2.
    mov eax, page_table_l2_2
    or eax, 0x03
    mov [page_table_l3 + 16], eax

    ; PDP[3] -> PD3.
    mov eax, page_table_l2_3
    or eax, 0x03
    mov [page_table_l3 + 24], eax


    ; -----------------------------------------------------------------------
    ; PD0: 0x00000000 - 0x3FFFFFFF
    ; -----------------------------------------------------------------------

    mov edi, page_table_l2_0
    mov eax, 0x00000083
    mov ecx, 512

.map0:
    mov [edi], eax
    add eax, 0x00200000
    add edi, 8
    loop .map0


    ; -----------------------------------------------------------------------
    ; PD1: 0x40000000 - 0x7FFFFFFF
    ; -----------------------------------------------------------------------

    mov edi, page_table_l2_1
    mov eax, 0x40000083
    mov ecx, 512

.map1:
    mov [edi], eax
    add eax, 0x00200000
    add edi, 8
    loop .map1


    ; -----------------------------------------------------------------------
    ; PD2: 0x80000000 - 0xBFFFFFFF
    ; -----------------------------------------------------------------------

    mov edi, page_table_l2_2
    mov eax, 0x80000083
    mov ecx, 512

.map2:
    mov [edi], eax
    add eax, 0x00200000
    add edi, 8
    loop .map2


    ; -----------------------------------------------------------------------
    ; PD3: 0xC0000000 - 0xFFFFFFFF
    ; -----------------------------------------------------------------------

    mov edi, page_table_l2_3
    mov eax, 0xC0000083
    mov ecx, 512

.map3:
    mov [edi], eax
    add eax, 0x00200000
    add edi, 8
    loop .map3

    ret


BITS 64

long_mode_start:

    ; Load the 64-bit data segment.
    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov fs, ax
    mov gs, ax

    ; Use a stack that is identity mapped.
    mov rsp, stack_top64

    ; Clear direction flag for C code.
    cld

    ; Pass the Multiboot2 information pointer to main64().
    mov edi, dword [multiboot_information]
    mov rdi, rdi

    ; Keep interrupts disabled until the kernel installs an IDT.
    cli

    call main64

.hang:
    cli
    hlt
    jmp .hang


section .rodata

align 8

gdt64:
    ; Null descriptor.
    dq 0x0000000000000000

    ; 64-bit code segment:
    ; present, ring 0, executable, readable, long mode.
    dq 0x00AF9A000000FFFF

    ; 64-bit data segment:
    ; present, ring 0, writable.
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
    resd 1

multiboot_information:
    resd 1
