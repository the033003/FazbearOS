#include "pci.h"
#include "io.h"

#define PCI_CONFIG_ADDRESS 0xCF8
#define PCI_CONFIG_DATA    0xCFC

uint32_t pci_read32(uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset)
{
    uint32_t address =
        (uint32_t)(0x80000000u |
                   ((uint32_t)bus  << 16) |
                   ((uint32_t)slot << 11) |
                   ((uint32_t)func <<  8) |
                   (offset & 0xFC));

    outl(PCI_CONFIG_ADDRESS, address);
    return inl(PCI_CONFIG_DATA);
}

uint16_t pci_read16(uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset)
{
    uint32_t data = pci_read32(bus, slot, func, offset);
    return (uint16_t)((data >> ((offset & 2) * 8)) & 0xFFFF);
}

uint8_t pci_read8(uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset)
{
    uint32_t data = pci_read32(bus, slot, func, offset);
    return (uint8_t)((data >> ((offset & 3) * 8)) & 0xFF);
}

void pci_write32(uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset, uint32_t value)
{
    uint32_t address =
        (uint32_t)(0x80000000u |
                   ((uint32_t)bus  << 16) |
                   ((uint32_t)slot << 11) |
                   ((uint32_t)func <<  8) |
                   (offset & 0xFC));

    outl(PCI_CONFIG_ADDRESS, address);
    outl(PCI_CONFIG_DATA, value);
}

void pci_write16(uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset, uint16_t value)
{
    uint32_t old = pci_read32(bus, slot, func, offset & 0xFC);
    uint32_t shift = (offset & 2) * 8;
    uint32_t mask  = 0xFFFFu << shift;
    uint32_t newv  = (old & ~mask) | ((uint32_t)value << shift);
    pci_write32(bus, slot, func, offset & 0xFC, newv);
}

int pci_find_uhci(uint16_t *io_base)
{
    for (int bus = 0; bus < 256; bus++) {
        for (uint8_t slot = 0; slot < 32; slot++) {
            for (uint8_t func = 0; func < 8; func++) {
                uint16_t vendor = pci_read16((uint8_t)bus, slot, func, 0x00);
                if (vendor == 0xFFFF) {
                    continue;
                }

                uint8_t class_code = pci_read8((uint8_t)bus, slot, func, 0x0B);
                uint8_t subclass   = pci_read8((uint8_t)bus, slot, func, 0x0A);
                uint8_t prog_if    = pci_read8((uint8_t)bus, slot, func, 0x09);

                if (class_code == 0x0C && subclass == 0x03 && prog_if == 0x00) {
                    uint32_t bar0 = pci_read32((uint8_t)bus, slot, func, 0x10);
                    if (bar0 & 1) {
                        *io_base = (uint16_t)(bar0 & ~0x3u);

                        uint16_t cmd = pci_read16((uint8_t)bus, slot, func, 0x04);
                        cmd |= 0x05; /* I/O + Bus Master */
                        pci_write16((uint8_t)bus, slot, func, 0x04, cmd);

                        return 1;
                    }
                }
            }
        }
    }
    return 0;
}