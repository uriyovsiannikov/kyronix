#pragma once
#include "cpu.h"

void idt_init(void);
void isr_dispatch(cpu_state_t* state);
