section .multiboot_header
align 8

header_start:
    ; Multiboot2 magic
    dd 0xE85250D6

    ; Architecture: i386 protected mode
    dd 0

    ; Header length
    dd header_end - header_start

    ; Checksum
    dd -(0xE85250D6 + 0 + (header_end - header_start))

    ; Multiboot2 end tag
    dw 0
    dw 0
    dd 8

header_end:
