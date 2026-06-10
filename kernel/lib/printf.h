#pragma once

#include <stdarg.h>
#include <stddef.h>

typedef void (*printf_putchar_fn)(char c, void* ctx);

int vprintf_cb(printf_putchar_fn fn, void* ctx, const char* fmt, va_list ap);

int snprintf(char* buf, size_t size, const char* fmt, ...) __attribute__((format(printf, 3, 4)));

int vsnprintf(char* buf, size_t size, const char* fmt, va_list ap);

void printf_set_putchar(printf_putchar_fn fn, void* ctx);

int kprintf(const char* fmt, ...) __attribute__((format(printf, 1, 2)));
