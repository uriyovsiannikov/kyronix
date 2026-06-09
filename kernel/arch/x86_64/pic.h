#pragma once

#include <stdint.h>
#include <stdbool.h>

#define PIC1_CMD  0x20
#define PIC1_DATA 0x21
#define PIC2_CMD  0xA0
#define PIC2_DATA 0xA1

#define PIC_BASE_MASTER 0x20
#define PIC_BASE_SLAVE  0x28

void pic_init(void);
void pic_set_mask(uint16_t irq);
void pic_clear_mask(uint16_t irq);
uint16_t pic_get_irr(void);
uint16_t pic_get_isr(void);
