global start
extern long_mode_start

section .text
bits 32

%define CODE_SEG 0x08
%define DATA_SEG 0x10

start:
    mov esp, stack_top

    call check_multiboot
    call check_cpuid
    call check_long_mode

    call setup_page_tables
    call enable_paging

    lgdt [gdt64.pointer]

    jmp CODE_SEG:long_mode_start


check_multiboot:
    cmp eax, 0x36d76289
    jne .no_multiboot
    ret

.no_multiboot:
    mov al, 'M'
    jmp error


check_cpuid:
    pushfd
    pop eax

    mov ecx, eax
    xor eax, 1 << 21

    push eax
    popfd

    pushfd
    pop eax

    push ecx
    popfd

    cmp eax, ecx
    je .no_cpuid

    ret

.no_cpuid:
    mov al, 'C'
    jmp error


check_long_mode:
    mov eax, 0x80000000
    cpuid

    cmp eax, 0x80000001
    jb .no_long_mode

    mov eax, 0x80000001
    cpuid

    test edx, 1 << 29
    jz .no_long_mode

    ret

.no_long_mode:
    mov al, 'L'
    jmp error


setup_page_tables:
    ; PML4 -> PDPT
    mov eax, page_table_l3
    or eax, 0x03
    mov [page_table_l4], eax

    ; PDPT -> page directory
    mov eax, page_table_l2
    or eax, 0x03
    mov [page_table_l3], eax

    ; Identity-map the first 1 GiB using 2 MiB pages.
    xor ecx, ecx

.map_loop:
    mov eax, 0x200000
    mul ecx

    or eax, 0x83
    mov [page_table_l2 + ecx * 8], eax

    inc ecx
    cmp ecx, 512
    jne .map_loop

    ret


enable_paging:
    ; Load PML4.
    mov eax, page_table_l4
    mov cr3, eax

    ; Enable Physical Address Extension.
    mov eax, cr4
    or eax, 1 << 5
    mov cr4, eax

    ; Enable Long Mode Enable in EFER.
    mov ecx, 0xC0000080
    rdmsr

    or eax, 1 << 8

    wrmsr

    ; Enable paging.
    mov eax, cr0
    or eax, 1 << 31
    mov cr0, eax

    ret


error:
    ; VGA text mode error display.
    mov word [0xB8000], 0x4F45
    mov word [0xB8002], 0x4F52
    mov word [0xB8004], 0x4F52
    mov word [0xB8006], 0x4F3A
    mov word [0xB8008], 0x4F20

    mov byte [0xB800A], al
    mov byte [0xB800B], 0x4F

.error_halt:
    cli
    hlt
    jmp .error_halt


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


section .rodata

align 8

gdt64:

    ; Null descriptor.
    dq 0

    ; 64-bit code segment.
    dq 0x00AF9A000000FFFF

    ; 64-bit data segment.
    dq 0x00AF92000000FFFF

.pointer:
    dw gdt64_end - gdt64 - 1
    dq gdt64

gdt64_end:
