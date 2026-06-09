#include "tty.h"
#include "serial.h"
#include "kbd.h"
#include "fb.h"
#include "../proc/proc.h"
#include "../arch/x86_64/cpu.h"

int64_t tty_read(char *buf, uint64_t len)
{
    if (!len) return 0;

    uint64_t i = 0;
    for (;;) {
        if (i >= len) break;

        if (serial_data_ready(COM1)) {
            uint8_t c = serial_getchar(COM1);
            if (c == '\r') c = '\n';
            if (c == 0x04) {
                if (i == 0) return 0;
                break;
            }
            buf[i++] = (char)c;
            continue;
        }

        if (kbd_data_ready()) {
            int c = kbd_getchar();
            if (c > 0) {
                if (c == '\r') c = '\n';
                if (c == 0x04) {
                    if (i == 0) return 0;
                    break;
                }
                buf[i++] = (char)(uint8_t)c;
            }
            continue;
        }

        if (i > 0) break;

        sched_yield_blocking();
        cpu_relax();
    }
    return (int64_t)i;
}

int64_t tty_write(const char *buf, uint64_t len)
{
    for (uint64_t i = 0; i < len; i++) {
        serial_putchar(COM1, buf[i]);
        if (g_fb.addr) fb_putchar(buf[i]);
    }
    return (int64_t)len;
}

bool tty_data_ready(void)
{
    return serial_data_ready(COM1) || kbd_data_ready();
}

void tty_putchar(char c)
{
    serial_putchar(COM1, c);
    if (g_fb.addr) fb_putchar(c);
}
