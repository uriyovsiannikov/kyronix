#pragma once
#include <stdint.h>
#include <stdbool.h>

void kbd_init(void);
bool kbd_data_ready(void);
int  kbd_getchar(void);
