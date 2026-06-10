#include "idt.h"
#include "gdt.h"
#include "lib/printf.h"
#include "pic.h"
#include "proc/proc.h"

#define IDT_INT_GATE 0x8E
#define IDT_TRAP_GATE 0x8F
#define IDT_USER_GATE 0xEE /* DPL=3: int 0x80 callable from ring 3 */

static idt_entry_t g_idt[256] __attribute__((aligned(16)));

extern uint64_t isr_stub_table[];

static void idt_set_gate(uint8_t vec, uint64_t handler, uint8_t type)
{
    g_idt[vec] = (idt_entry_t) {
        .offset_low = (uint16_t) (handler & 0xFFFF),
        .selector = GDT_KERNEL_CODE,
        .ist = 0,
        .type_attr = type,
        .offset_mid = (uint16_t) ((handler >> 16) & 0xFFFF),
        .offset_high = (uint32_t) (handler >> 32),
        .zero = 0,
    };
}

void idt_init(void)
{
    pic_remap(32, 40);
    pic_mask_all();

    for (uint8_t i = 0; i < 32; i++)
        idt_set_gate(i, isr_stub_table[i], IDT_INT_GATE);

    idt_set_gate(3, isr_stub_table[3], IDT_TRAP_GATE); /* #BP: trap so debugger can resume */

    for (uint8_t i = 0; i < 16; i++)
        idt_set_gate(32 + i, isr_stub_table[32 + i], IDT_INT_GATE);

    idt_set_gate(0x80, isr_stub_table[48], IDT_USER_GATE);

    struct
    {
        uint16_t limit;
        uint64_t base;
    } __attribute__((packed)) idtr = {
        .limit = (uint16_t) (sizeof(g_idt) - 1),
        .base = (uint64_t) g_idt,
    };
    __asm__ volatile("lidt %0" ::"m"(idtr) : "memory");
}

static const char* const exc_name[] = {
    "#DE Divide Error",
    "#DB Debug",
    "NMI",
    "#BP Breakpoint",
    "#OF Overflow",
    "#BR Bound Range Exceeded",
    "#UD Invalid Opcode",
    "#NM Device Not Available",
    "#DF Double Fault",
    "Coproc Segment Overrun",
    "#TS Invalid TSS",
    "#NP Segment Not Present",
    "#SS Stack Segment Fault",
    "#GP General Protection Fault",
    "#PF Page Fault",
    "(reserved 15)",
    "#MF x87 FP Exception",
    "#AC Alignment Check",
    "#MC Machine Check",
    "#XF SIMD FP Exception",
    "(reserved 20)",
    "(reserved 21)",
    "(reserved 22)",
    "(reserved 23)",
    "(reserved 24)",
    "(reserved 25)",
    "(reserved 26)",
    "(reserved 27)",
    "(reserved 28)",
    "(reserved 29)",
    "#SX Security Exception",
    "(reserved 31)",
};

void isr_dispatch(cpu_state_t* state)
{
    uint64_t n = state->int_no;

    if (n < 32)
    {
        kprintf("\n\n!!! KERNEL EXCEPTION !!! pid=%u\n", g_current_proc ? g_current_proc->pid : 0);
        kprintf("  %s  (vector %lu)\n", exc_name[n], n);
        kprintf("  error = 0x%016lx\n", state->error_code);
        kprintf("  RIP   = 0x%016lx   CS     = 0x%04lx\n", state->rip, state->cs);
        kprintf("  RFLAGS= 0x%016lx\n", state->rflags);
        kprintf("  RAX   = 0x%016lx   RBX    = 0x%016lx\n", state->rax, state->rbx);
        kprintf("  RCX   = 0x%016lx   RDX    = 0x%016lx\n", state->rcx, state->rdx);
        kprintf("  RSI   = 0x%016lx   RDI    = 0x%016lx\n", state->rsi, state->rdi);
        kprintf("  R8    = 0x%016lx   R9     = 0x%016lx\n", state->r8, state->r9);
        kprintf("  R10   = 0x%016lx   R11    = 0x%016lx\n", state->r10, state->r11);
        kprintf("  R12   = 0x%016lx   R13    = 0x%016lx\n", state->r12, state->r13);
        kprintf("  R14   = 0x%016lx   R15    = 0x%016lx\n", state->r14, state->r15);
        kprintf("  RBP   = 0x%016lx\n", state->rbp);

        if (n == 14)
        { /* page fault: CR2 = faulting address */
            uint64_t cr2;
            __asm__ volatile("mov %%cr2, %0" : "=r"(cr2));
            kprintf("  CR2   = 0x%016lx  (fault address)\n", cr2);
            kprintf("  PF flags: %s%s%s%s%s\n", state->error_code & 1 ? "present " : "non-present ",
                    state->error_code & 2 ? "write " : "read ",
                    state->error_code & 4 ? "user " : "kernel ",
                    state->error_code & 8 ? "reserved-bit " : "",
                    state->error_code & 16 ? "ifetch" : "");
        }

        /* rsp/ss are only valid if we came from ring 3 */
        if ((state->cs & 3) == 3)
            kprintf("  RSP   = 0x%016lx   SS     = 0x%04lx  (ring-3)\n", state->rsp, state->ss);

        cpu_halt();
    }
    else if (n < 48)
    {
        pic_send_eoi((uint8_t) (n - 32));
    }
}
