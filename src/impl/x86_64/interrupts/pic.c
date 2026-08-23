#include "pic.h"
#include "io.h"

#define PIC1_COMMAND 0x20
#define PIC1_DATA    0x21

#define PIC2_COMMAND 0xA0
#define PIC2_DATA    0xA1

#define PIC_EOI      0x20

#define ICW1_ICW4   0x01
#define ICW1_INIT   0x10

#define ICW4_8086   0x01

void pic_init(void)
{
    uint8_t master_mask = inb(PIC1_DATA);
    uint8_t slave_mask = inb(PIC2_DATA);

    outb(PIC1_COMMAND, ICW1_INIT | ICW1_ICW4);
    io_wait();

    outb(PIC2_COMMAND, ICW1_INIT | ICW1_ICW4);
    io_wait();

    outb(PIC1_DATA, 32);
    io_wait();

    outb(PIC2_DATA, 40);
    io_wait();

    outb(PIC1_DATA, 4);
    io_wait();

    outb(PIC2_DATA, 2);
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

    outb(PIC1_DATA, master_mask);
    outb(PIC2_DATA, slave_mask);

    /*
     * Only timer (IRQ0) and keyboard (IRQ1)
     * are currently implemented.
     */
    pic_mask_all();

    pic_unmask(0);
    pic_unmask(1);
}

void pic_mask_all(void)
{
    outb(PIC1_DATA, 0xFF);
    outb(PIC2_DATA, 0xFF);
}

void pic_unmask(uint8_t irq)
{
    if (irq < 8) {
        uint8_t mask = inb(PIC1_DATA);

        mask &= (uint8_t)~(1u << irq);

        outb(PIC1_DATA, mask);
        return;
    }

    irq -= 8;

    uint8_t mask = inb(PIC2_DATA);

    mask &= (uint8_t)~(1u << irq);

    outb(PIC2_DATA, mask);
}

void pic_mask(uint8_t irq)
{
    if (irq < 8) {
        uint8_t mask = inb(PIC1_DATA);

        mask |= (uint8_t)(1u << irq);

        outb(PIC1_DATA, mask);
        return;
    }

    irq -= 8;

    uint8_t mask = inb(PIC2_DATA);

    mask |= (uint8_t)(1u << irq);

    outb(PIC2_DATA, mask);
}
