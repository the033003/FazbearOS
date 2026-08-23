#include "pic.h"
#include "io.h"

#define PIC1_COMMAND 0x20
#define PIC1_DATA    0x21

#define PIC2_COMMAND 0xA0
#define PIC2_DATA    0xA1

#define ICW1_ICW4    0x01
#define ICW1_INIT    0x10
#define ICW4_8086    0x01

void pic_init(void)
{
    /*
     * Initialize master PIC.
     */
    outb(
        PIC1_COMMAND,
        ICW1_INIT | ICW1_ICW4
    );

    io_wait();

    /*
     * Initialize slave PIC.
     */
    outb(
        PIC2_COMMAND,
        ICW1_INIT | ICW1_ICW4
    );

    io_wait();

    /*
     * Master IRQs -> vectors 32-39.
     */
    outb(
        PIC1_DATA,
        32
    );

    io_wait();

    /*
     * Slave IRQs -> vectors 40-47.
     */
    outb(
        PIC2_DATA,
        40
    );

    io_wait();

    /*
     * Slave is connected to master IRQ2.
     */
    outb(
        PIC1_DATA,
        0x04
    );

    io_wait();

    /*
     * Slave identity = 2.
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
     * Mask everything initially.
     */
    pic_mask_all();

    /*
     * IRQ0 = timer.
     */
    pic_unmask(0);

    /*
     * IRQ1 = keyboard.
     */
    pic_unmask(1);

    /*
     * IRQ12 = PS/2 mouse.
     */
    pic_unmask(12);

    /*
     * IRQ12 is behind the slave PIC.
     * Therefore IRQ2 on the master must be open.
     */
    pic_unmask(2);
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
