#include "pic.h"
#include "io.h"

#define PIC1_COMMAND 0x20
#define PIC1_DATA    0x21

#define PIC2_COMMAND 0xA0
#define PIC2_DATA    0xA1

#define PIC_EOI      0x20

#define ICW1_ICW4    0x01
#define ICW1_INIT    0x10
#define ICW4_8086    0x01

void pic_init(void)
{
    uint8_t master_mask =
        inb(PIC1_DATA);

    uint8_t slave_mask =
        inb(PIC2_DATA);

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
     * Master PIC:
     * IRQ0-7 -> vectors 32-39
     */
    outb(
        PIC1_DATA,
        32
    );

    io_wait();

    /*
     * Slave PIC:
     * IRQ8-15 -> vectors 40-47
     */
    outb(
        PIC2_DATA,
        40
    );

    io_wait();

    /*
     * Tell master that a slave exists
     * on IRQ2.
     */
    outb(
        PIC1_DATA,
        4
    );

    io_wait();

    /*
     * Tell slave its cascade identity.
     */
    outb(
        PIC2_DATA,
        2
    );

    io_wait();

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
     * Start with everything masked.
     *
     * We explicitly choose which devices
     * FazbearOS currently accepts.
     */
    pic_mask_all();

    /*
     * IRQ0 = PIT timer.
     * IRQ1 = PS/2 keyboard.
     * IRQ12 = PS/2 mouse.
     */
    pic_unmask(0);
    pic_unmask(1);
    pic_unmask(12);

    /*
     * IRQ12 lives behind the slave PIC,
     * so the master's cascade IRQ2 must also
     * be enabled.
     */
    pic_unmask(2);

    /*
     * Preserve the constants above as part
     * of the PIC initialization contract.
     */
    (void)master_mask;
    (void)slave_mask;
    (void)PIC_EOI;
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

void pic_unmask(
    uint8_t irq
)
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

void pic_mask(
    uint8_t irq
)
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
