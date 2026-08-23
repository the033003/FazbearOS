#include "pic.h"

#include "io.h"

#define PIC1_COMMAND 0x20
#define PIC1_DATA    0x21

#define PIC2_COMMAND 0xA0
#define PIC2_DATA    0xA1

#define ICW1_ICW4 0x01
#define ICW1_INIT 0x10
#define ICW4_8086 0x01

void pic_init(void)
{
    /*
     * Begin initialization sequence.
     */
    outb(
        PIC1_COMMAND,
        ICW1_INIT | ICW1_ICW4
    );

    io_wait();

    outb(
        PIC2_COMMAND,
        ICW1_INIT | ICW1_ICW4
    );

    io_wait();

    /*
     * Remap:
     *
     * Master IRQ0-7  -> vectors 32-39
     * Slave  IRQ8-15 -> vectors 40-47
     */
    outb(
        PIC1_DATA,
        32
    );

    io_wait();

    outb(
        PIC2_DATA,
        40
    );

    io_wait();

    /*
     * Slave PIC is connected to master IRQ2.
     */
    outb(
        PIC1_DATA,
        0x04
    );

    io_wait();

    /*
     * Slave identity is IRQ2.
     */
    outb(
        PIC2_DATA,
        0x02
    );

    io_wait();

    /*
     * 8086/88 mode.
     */
    outb(
        PIC1_DATA,
        ICW4_8086
    );

    io_wait();

    outb(
        PIC2_DATA,
        ICW4_8086
    );

    io_wait();

    /*
     * Begin with all IRQs masked.
     */
    pic_mask_all();

    /*
     * Enable:
     *
     * IRQ0  timer
     * IRQ1  keyboard
     * IRQ2  slave cascade
     * IRQ12 mouse
     */
    pic_unmask(0);
    pic_unmask(1);
    pic_unmask(2);
    pic_unmask(12);
}

void pic_mask_all(void)
{
    outb(
        PIC1_DATA,
        0xFF
    );

    outb(
        PIC2_DATA,
        0xFF
    );
}

void pic_unmask(uint8_t irq)
{
    if (irq < 8) {

        uint8_t mask =
            inb(PIC1_DATA);

        mask &=
            (uint8_t)~(
                1u << irq
            );

        outb(
            PIC1_DATA,
            mask
        );

        return;
    }

    irq -= 8;

    uint8_t mask =
        inb(PIC2_DATA);

    mask &=
        (uint8_t)~(
            1u << irq
        );

    outb(
        PIC2_DATA,
        mask
    );
}

void pic_mask(uint8_t irq)
{
    if (irq < 8) {

        uint8_t mask =
            inb(PIC1_DATA);

        mask |=
            (uint8_t)(
                1u << irq
            );

        outb(
            PIC1_DATA,
            mask
        );

        return;
    }

    irq -= 8;

    uint8_t mask =
        inb(PIC2_DATA);

    mask |=
        (uint8_t)(
            1u << irq
        );

    outb(
        PIC2_DATA,
        mask
    );
}
