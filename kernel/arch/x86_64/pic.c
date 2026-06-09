#include "pic.h"
#include "cpu.h"

#define ICW1_ICW4      0x01
#define ICW1_SINGLE    0x02
#define ICW1_INTERVAL4 0x04
#define ICW1_LEVEL     0x08
#define ICW1_INIT      0x10

#define ICW4_8086      0x01
#define ICW4_AUTO      0x02
#define ICW4_BUF_SLAVE 0x08
#define ICW4_BUF_MASTER 0x0C
#define ICW4_SFNM      0x10

#define PIC_EOI        0x20

static uint16_t pic_mask = 0xFFFF;

void pic_init(void)
{
    /* save masks */
    uint8_t a1 = inb(PIC1_DATA);
    uint8_t a2 = inb(PIC2_DATA);

    /* start init sequence (cascade, ICW4 needed) */
    outb(PIC1_CMD, ICW1_INIT | ICW1_ICW4);
    io_wait();
    outb(PIC2_CMD, ICW1_INIT | ICW1_ICW4);
    io_wait();

    /* ICW2: vector base */
    outb(PIC1_DATA, PIC_BASE_MASTER);
    io_wait();
    outb(PIC2_DATA, PIC_BASE_SLAVE);
    io_wait();

    /* ICW3: cascade wiring */
    outb(PIC1_DATA, 0x04);  /* slave at IRQ2 */
    io_wait();
    outb(PIC2_DATA, 0x02);  /* cascade identity */
    io_wait();

    /* ICW4: 8086 mode */
    outb(PIC1_DATA, ICW4_8086);
    io_wait();
    outb(PIC2_DATA, ICW4_8086);
    io_wait();

    /* restore saved masks — all IRQs masked */
    outb(PIC1_DATA, a1);
    outb(PIC2_DATA, a2);

    /* explicitly mask all to be safe */
    pic_mask = 0xFFFF;
    outb(PIC1_DATA, pic_mask & 0xFF);
    outb(PIC2_DATA, (pic_mask >> 8) & 0xFF);
}

void pic_set_mask(uint16_t irq)
{
    pic_mask |= (1 << irq);
    if (irq < 8)
        outb(PIC1_DATA, pic_mask & 0xFF);
    else
        outb(PIC2_DATA, (pic_mask >> 8) & 0xFF);
}

void pic_clear_mask(uint16_t irq)
{
    pic_mask &= ~(1 << irq);
    if (irq < 8)
        outb(PIC1_DATA, pic_mask & 0xFF);
    else
        outb(PIC2_DATA, (pic_mask >> 8) & 0xFF);
}

uint16_t pic_get_irr(void)
{
    outb(PIC1_CMD, 0x0A);
    outb(PIC2_CMD, 0x0A);
    return (inb(PIC2_CMD) << 8) | inb(PIC1_CMD);
}

uint16_t pic_get_isr(void)
{
    outb(PIC1_CMD, 0x0B);
    outb(PIC2_CMD, 0x0B);
    return (inb(PIC2_CMD) << 8) | inb(PIC1_CMD);
}
