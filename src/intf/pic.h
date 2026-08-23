#pragma once

#include <stdint.h>

void pic_init(void);
void pic_mask_all(void);
void pic_unmask(uint8_t irq);
void pic_mask(uint8_t irq);
