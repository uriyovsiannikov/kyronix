#pragma once
#include <stdint.h>

extern volatile uint64_t g_ticks;

void pit_init(void);
