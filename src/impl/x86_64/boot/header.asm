section .multiboot_header

align 8

multiboot_header_start:

    ; Multiboot2 header magic.
    dd 0xE85250D6

    ; Architecture: i386.
    dd 0

    ; Header length.
    dd multiboot_header_end - multiboot_header_start

    ; Checksum.
    dd -(0xE85250D6 + 0 + (multiboot_header_end - multiboot_header_start))


    ; ------------------------------------------------------------
    ; Framebuffer tag
    ;
    ; Type     = 5
    ; Flags    = 0
    ; Size     = 20
    ; Width    = 1024
    ; Height   = 768
    ; Depth    = 32
    ; ------------------------------------------------------------

    align 8

    dw 5
    dw 0
    dd 20
    dd 1024
    dd 768
    dd 32


    ; ------------------------------------------------------------
    ; End tag
    ; ------------------------------------------------------------

    align 8

    dw 0
    dw 0
    dd 8

multiboot_header_end:
