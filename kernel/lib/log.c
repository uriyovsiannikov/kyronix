#include "lib/log.h"
#include "drivers/serial.h"
#include "lib/printf.h"
#include <stdarg.h>

static void log_putchar(char c, void* ctx)
{
    (void) ctx;
    serial_putchar(COM1, c);
}

void klog_printf(const char* fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    vprintf_cb(log_putchar, NULL, fmt, ap);
    va_end(ap);
}
