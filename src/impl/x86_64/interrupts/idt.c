#include "interrupts.h"

#include "io.h"
#include "mouse.h"
#include "print.h"

struct idt_entry {
    uint16_t offset_low;
    uint16_t selector;
    uint8_t ist;
    uint8_t type_attributes;
    uint16_t offset_middle;
    uint32_t offset_high;
    uint32_t reserved;
} __attribute__((packed));

struct idt_pointer {
    uint16_t limit;
    uint64_t base;
} __attribute__((packed));

extern void isr0(void);
extern void isr1(void);
extern void isr2(void);
extern void isr3(void);
extern void isr4(void);
extern void isr5(void);
extern void isr6(void);
extern void isr7(void);
extern void isr8(void);
extern void isr9(void);
extern void isr10(void);
extern void isr11(void);
extern void isr12(void);
extern void isr13(void);
extern void isr14(void);
extern void isr15(void);
extern void isr16(void);
extern void isr17(void);
extern void isr18(void);
extern void isr19(void);
extern void isr20(void);
extern void isr21(void);
extern void isr22(void);
extern void isr23(void);
extern void isr24(void);
extern void isr25(void);
extern void isr26(void);
extern void isr27(void);
extern void isr28(void);
extern void isr29(void);
extern void isr30(void);
extern void isr31(void);

extern void irq0(void);
extern void irq1(void);
extern void irq12(void);

static struct idt_entry idt[256];

static void idt_set_gate(
    uint8_t vector,
    void (*handler)(void)
)
{
    uint64_t address =
        (uint64_t)handler;

    idt[vector].offset_low =
        (uint16_t)(
            address &
            0xFFFF
        );

    idt[vector].selector =
        0x08;

    idt[vector].ist =
        0;

    idt[vector].type_attributes =
        0x8E;

    idt[vector].offset_middle =
        (uint16_t)(
            (address >> 16) &
            0xFFFF
        );

    idt[vector].offset_high =
        (uint32_t)(
            (address >> 32) &
            0xFFFFFFFF
        );

    idt[vector].reserved =
        0;
}

static void idt_load(void)
{
    struct idt_pointer pointer;

    pointer.limit =
        sizeof(idt) - 1;

    pointer.base =
        (uint64_t)&idt[0];

    __asm__ volatile (
        "lidt %0"
        :
        : "m"(pointer)
    );
}

static void idt_clear(void)
{
    for (size_t i = 0;
         i < 256;
         i++) {

        idt[i] =
            (struct idt_entry) {
                0
            };
    }
}

void irq_end_of_interrupt(
    uint8_t irq
)
{
    /*
     * IRQ8-IRQ15 originate on the slave PIC.
     *
     * The slave receives EOI first, followed by the master.
     */
    if (irq >= 8) {

        outb(
            0xA0,
            0x20
        );
    }

    outb(
        0x20,
        0x20
    );
}

void interrupts_init(void)
{
    idt_clear();

    idt_set_gate(0, isr0);
    idt_set_gate(1, isr1);
    idt_set_gate(2, isr2);
    idt_set_gate(3, isr3);
    idt_set_gate(4, isr4);
    idt_set_gate(5, isr5);
    idt_set_gate(6, isr6);
    idt_set_gate(7, isr7);
    idt_set_gate(8, isr8);
    idt_set_gate(9, isr9);
    idt_set_gate(10, isr10);
    idt_set_gate(11, isr11);
    idt_set_gate(12, isr12);
    idt_set_gate(13, isr13);
    idt_set_gate(14, isr14);
    idt_set_gate(15, isr15);
    idt_set_gate(16, isr16);
    idt_set_gate(17, isr17);
    idt_set_gate(18, isr18);
    idt_set_gate(19, isr19);
    idt_set_gate(20, isr20);
    idt_set_gate(21, isr21);
    idt_set_gate(22, isr22);
    idt_set_gate(23, isr23);
    idt_set_gate(24, isr24);
    idt_set_gate(25, isr25);
    idt_set_gate(26, isr26);
    idt_set_gate(27, isr27);
    idt_set_gate(28, isr28);
    idt_set_gate(29, isr29);
    idt_set_gate(30, isr30);
    idt_set_gate(31, isr31);

    /*
     * Hardware IRQs.
     */
    idt_set_gate(
        32,
        irq0
    );

    idt_set_gate(
        33,
        irq1
    );

    /*
     * IRQ12 -> PIC vector 44.
     */
    idt_set_gate(
        44,
        irq12
    );

    idt_load();

    /*
     * Do NOT enable interrupts here.
     * STI is done in kernel_main after mouse_init() finishes.
     */
}

static const char* exception_name(
    uint64_t vector
)
{
    static const char* names[] = {
        "Divide Error",
        "Debug",
        "Non-Maskable Interrupt",
        "Breakpoint",
        "Overflow",
        "BOUND Range Exceeded",
        "Invalid Opcode",
        "Device Not Available",
        "Double Fault",
        "Coprocessor Segment Overrun",
        "Invalid TSS",
        "Segment Not Present",
        "Stack-Segment Fault",
        "General Protection Fault",
        "Page Fault",
        "Reserved",
        "x87 Floating-Point Exception",
        "Alignment Check",
        "Machine Check",
        "SIMD Floating-Point Exception",
        "Virtualization Exception",
        "Control Protection Exception",
        "Reserved",
        "Reserved",
        "Reserved",
        "Reserved",
        "Reserved",
        "Hypervisor Injection Exception",
        "VMM Communication Exception",
        "Security Exception",
        "Reserved",
        "Reserved"
    };

    if (vector >= 32) {
        return "Unknown";
    }

    return names[vector];
}

static void print_hex(
    uint64_t value
)
{
    static const char digits[] =
        "0123456789ABCDEF";

    print_str(
        "0x"
    );

    int started = 0;

    for (int shift = 60;
         shift >= 0;
         shift -= 4) {

        uint8_t digit =
            (uint8_t)(
                (value >> shift) &
                0xF
            );

        if (digit != 0 ||
            started ||
            shift == 0) {

            print_char(
                digits[digit]
            );

            started = 1;
        }
    }
}

void interrupt_dispatch(
    struct interrupt_frame* frame
)
{
    if (frame == 0) {
        return;
    }

    /*
     * Hardware IRQs use vectors 32-47.
     */
    if (frame->vector >= 32 &&
        frame->vector <= 47) {

        uint8_t irq =
            (uint8_t)(
                frame->vector - 32
            );

        if (irq == 0) {

            extern void timer_tick(void);

            timer_tick();

        } else if (irq == 1) {

            extern void keyboard_interrupt(void);

            keyboard_interrupt();

        } else if (irq == 12) {

            /*
             * Let the mouse driver inspect the controller
             * status and consume the auxiliary byte.
             */
            mouse_interrupt();
        }

        irq_end_of_interrupt(
            irq
        );

        return;
    }

    /*
     * CPU exception.
     */
    if (frame->vector < 32) {

        __asm__ volatile (
            "cli"
        );

        print_set_color(
            PRINT_COLOR_LIGHT_RED,
            PRINT_COLOR_BLACK
        );

        print_clear();

        print_str(
            "\n"
            "================ KERNEL PANIC ================\n"
            "\n"
        );

        print_str(
            "CPU EXCEPTION: "
        );

        print_str(
            exception_name(
                frame->vector
            )
        );

        print_char(
            '\n'
        );

        print_str(
            "VECTOR: "
        );

        print_hex(
            frame->vector
        );

        print_char(
            '\n'
        );

        print_str(
            "ERROR:  "
        );

        print_hex(
            frame->error_code
        );

        print_char(
            '\n'
        );

        print_str(
            "RIP:    "
        );

        print_hex(
            frame->rip
        );

        print_char(
            '\n'
        );

        print_str(
            "RFLAGS: "
        );

        print_hex(
            frame->rflags
        );

        print_char(
            '\n'
        );

        print_str(
            "\n"
            "The kernel has halted to prevent further corruption.\n"
        );

        print_set_color(
            PRINT_COLOR_WHITE,
            PRINT_COLOR_BLACK
        );

        for (;;) {

            __asm__ volatile (
                "hlt"
            );
        }
    }
}