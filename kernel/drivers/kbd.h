#pragma once
#include <stdbool.h>
#include <stdint.h>

void kbd_init(void);
bool kbd_data_ready(void);
int kbd_getchar(void);
