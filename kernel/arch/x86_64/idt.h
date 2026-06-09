#pragma once

#include <stdint.h>
#include "cpu.h"

#define IDT_ENTRIES 256

#define IDT_TYPE_INTERRUPT 0x8E  /* present, dpl=0, 64-bit interrupt gate */
#define IDT_TYPE_TRAP      0x8F
#define IDT_TYPE_USER      0xEE  /* present, dpl=3, 64-bit interrupt gate */

typedef void (*isr_handler_t)(cpu_state_t *state);

extern void *isr_stub_table[IDT_ENTRIES];

void idt_init(void);
void isr_register_handler(uint8_t vector, isr_handler_t handler);
