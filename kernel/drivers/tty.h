#pragma once
#include <stdint.h>
#include <stdbool.h>

int64_t tty_read(char *buf, uint64_t len);
int64_t tty_write(const char *buf, uint64_t len);
bool    tty_data_ready(void);
void    tty_putchar(char c);
