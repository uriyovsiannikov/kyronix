#include "idt.h"
#include "gdt.h"
#include "drivers/serial.h"
#include "lib/printf.h"

static idt_entry_t idt[IDT_ENTRIES] __attribute__((aligned(0x10)));
static idtr_t      idtr;

static isr_handler_t handlers[IDT_ENTRIES];

static void idt_set_entry(int vec, void *handler, uint8_t type)
{
    uint64_t addr = (uint64_t)handler;
    idt[vec].offset_low  = addr & 0xFFFF;
    idt[vec].selector    = GDT_CS_SEL;
    idt[vec].ist         = 0;
    idt[vec].type_attr   = type;
    idt[vec].offset_mid  = (addr >> 16) & 0xFFFF;
    idt[vec].offset_high = (addr >> 32);
    idt[vec].zero        = 0;
}

static const char *exception_names[32] = {
    [0]  = "#DE — divide by zero",
    [1]  = "#DB — debug",
    [2]  = "#NMI — non-maskable",
    [3]  = "#BP — breakpoint",
    [4]  = "#OF — overflow",
    [5]  = "#BR — bound range",
    [6]  = "#UD — invalid opcode",
    [7]  = "#NM — device not available",
    [8]  = "#DF — double fault",
    [9]  = "#FPU — coprocessor segment overrun",
    [10] = "#TS — invalid TSS",
    [11] = "#NP — segment not present",
    [12] = "#SS — stack segment",
    [13] = "#GP — general protection",
    [14] = "#PF — page fault",
    [15] = "(reserved 15)",
    [16] = "#MF — x87 floating-point",
    [17] = "#AC — alignment check",
    [18] = "#MC — machine check",
    [19] = "#XM — SIMD floating-point",
    [20] = "#VE — virtualization",
    [21] = "#CP — control protection",
    [22] = "(reserved 22)",
    [23] = "(reserved 23)",
    [24] = "(reserved 24)",
    [25] = "(reserved 25)",
    [26] = "(reserved 26)",
    [27] = "(reserved 27)",
    [28] = "(reserved 28)",
    [29] = "(reserved 29)",
    [30] = "#SX — security",
    [31] = "(reserved 31)",
};

static void default_handler(cpu_state_t *state)
{
    if (state->int_no < 32) {
        kprintf("\n[PANIC] %s\n", exception_names[state->int_no]
                ? exception_names[state->int_no] : "unknown exception");
    } else {
        kprintf("\n[PANIC] unhandled interrupt #%lu\n", state->int_no);
    }
    kprintf("  RIP=0x%016lx  CS=0x%04lx  RFLAGS=0x%016lx\n",
            state->rip, state->cs, state->rflags);
    kprintf("  RSP=0x%016lx  SS=0x%04lx  error=0x%016lx\n",
            state->rsp, state->ss, state->error_code);

    cpu_halt();
}

void isr_handler(cpu_state_t *state)
{
    isr_handler_t h = handlers[state->int_no];
    if (h)
        h(state);
    else
        default_handler(state);
}

void isr_register_handler(uint8_t vector, isr_handler_t handler)
{
    handlers[vector] = handler;
}

void idt_init(void)
{
    for (int i = 0; i < IDT_ENTRIES; i++)
        idt_set_entry(i, isr_stub_table[i], IDT_TYPE_INTERRUPT);

    idtr.limit = sizeof(idt) - 1;
    idtr.base  = (uint64_t)&idt;

    __asm__ volatile("lidt %0" : : "m"(idtr) : "memory");
}
