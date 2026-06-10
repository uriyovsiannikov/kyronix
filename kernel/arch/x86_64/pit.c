#include "pit.h"
#include "cpu.h"
#include "pic.h"

#define PIT_CHANNEL0 0x40
#define PIT_CMD      0x43
#define PIT_DIVISOR  1193  /* 1193182 Hz / 1193 ≈ 1000 Hz */

volatile uint64_t g_ticks = 0;

void pit_init(void)
{
    outb(PIT_CMD, 0x36);  /* channel 0, lo/hi, mode 2 (rate gen), binary */
    outb(PIT_CHANNEL0, (uint8_t)(PIT_DIVISOR & 0xFF));
    outb(PIT_CHANNEL0, (uint8_t)(PIT_DIVISOR >> 8));
    pic_unmask_irq(0);
}
