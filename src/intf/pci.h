#ifndef FAZBEAROS_PCI_H
#define FAZBEAROS_PCI_H

#include <stdint.h>

uint32_t pci_read32(uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset);
uint16_t pci_read16(uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset);
uint8_t  pci_read8 (uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset);

void pci_write32(uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset, uint32_t value);
void pci_write16(uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset, uint16_t value);

int pci_find_uhci(uint16_t *io_base);

#endif