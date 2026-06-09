#include "kbd.h"
#include "../arch/x86_64/cpu.h"
#include "../lib/log.h"

#define KBD_DATA  0x60
#define KBD_STAT  0x64
#define KBS_OBF   (1u << 0)

static bool g_shift, g_ctrl, g_alt, g_caps;
static bool g_ext;     /* got 0xE0 prefix, next byte completes the scan code */

static const char sc_ascii[128] = {
    [0x01] = 0x1B, [0x02] = '1',  [0x03] = '2',  [0x04] = '3',
    [0x05] = '4',  [0x06] = '5',  [0x07] = '6',  [0x08] = '7',
    [0x09] = '8',  [0x0A] = '9',  [0x0B] = '0',  [0x0C] = '-',
    [0x0D] = '=',  [0x0E] = 0x08, [0x0F] = 0x09,
    [0x10] = 'q',  [0x11] = 'w',  [0x12] = 'e',  [0x13] = 'r',
    [0x14] = 't',  [0x15] = 'y',  [0x16] = 'u',  [0x17] = 'i',
    [0x18] = 'o',  [0x19] = 'p',  [0x1A] = '[',  [0x1B] = ']',
    [0x1C] = 0x0A,
    [0x1E] = 'a',  [0x1F] = 's',  [0x20] = 'd',  [0x21] = 'f',
    [0x22] = 'g',  [0x23] = 'h',  [0x24] = 'j',  [0x25] = 'k',
    [0x26] = 'l',  [0x27] = ';',  [0x28] = '\'', [0x29] = '`',
    [0x2B] = '\\',
    [0x2C] = 'z',  [0x2D] = 'x',  [0x2E] = 'c',  [0x2F] = 'v',
    [0x30] = 'b',  [0x31] = 'n',  [0x32] = 'm',  [0x33] = ',',
    [0x34] = '.',  [0x35] = '/',
    [0x37] = '*',
    [0x39] = ' ',  [0x4A] = '-',  [0x4E] = '+',
};

static const char sc_shift_ascii[128] = {
    [0x01] = 0x1B, [0x02] = '!',  [0x03] = '@',  [0x04] = '#',
    [0x05] = '$',  [0x06] = '%',  [0x07] = '^',  [0x08] = '&',
    [0x09] = '*',  [0x0A] = '(',  [0x0B] = ')',  [0x0C] = '_',
    [0x0D] = '+',  [0x0E] = 0x08, [0x0F] = 0x09,
    [0x10] = 'Q',  [0x11] = 'W',  [0x12] = 'E',  [0x13] = 'R',
    [0x14] = 'T',  [0x15] = 'Y',  [0x16] = 'U',  [0x17] = 'I',
    [0x18] = 'O',  [0x19] = 'P',  [0x1A] = '{',  [0x1B] = '}',
    [0x1C] = 0x0A,
    [0x1E] = 'A',  [0x1F] = 'S',  [0x20] = 'D',  [0x21] = 'F',
    [0x22] = 'G',  [0x23] = 'H',  [0x24] = 'J',  [0x25] = 'K',
    [0x26] = 'L',  [0x27] = ':',  [0x28] = '"',  [0x29] = '~',
    [0x2B] = '|',
    [0x2C] = 'Z',  [0x2D] = 'X',  [0x2E] = 'C',  [0x2F] = 'V',
    [0x30] = 'B',  [0x31] = 'N',  [0x32] = 'M',  [0x33] = '<',
    [0x34] = '>',  [0x35] = '?',
    [0x37] = '*',
    [0x39] = ' ',  [0x4A] = '-',  [0x4E] = '+',
};

void kbd_init(void)
{
    /* PS/2 controller: enable keyboard port (port 2 is already disabled by BIOS).
     * Command 0xAE = enable first PS/2 port (keyboard). */
    while (inb(KBD_STAT) & 2) cpu_relax();
    outb(KBD_STAT, 0xAE);

    /* flush any leftover data */
    while (inb(KBD_STAT) & KBS_OBF) inb(KBD_DATA);

    log_info("PS/2 keyboard: enabled");
}

int kbd_getchar(void)
{
    if (!(inb(KBD_STAT) & KBS_OBF)) return -1;

    int raw = inb(KBD_DATA);
    if (raw < 0) return -1;

    uint8_t sc = (uint8_t)raw;

    /* extended prefix (arrow keys, etc.) */
    if (sc == 0xE0) {
        g_ext = true;
        return -1;
    }

    bool release = sc & 0x80;
    sc &= 0x7F;

    if (release) {
        switch (sc) {
            case 0x2A: case 0x36: g_shift = false; break;
            case 0x1D: g_ctrl  = false; break;
            case 0x38: g_alt   = false; break;
        }
        g_ext = false;
        return -1;
    }

    switch (sc) {
        case 0x2A: case 0x36: g_shift = true;  return -1;
        case 0x1D: g_ctrl  = true;  return -1;
        case 0x38: g_alt   = true;  return -1;
        case 0x3A: g_caps  = !g_caps; return -1;
        case 0x3B ... 0x44:  return -1;
        case 0x45 ... 0x46:  return -1;
        case 0x47 ... 0x53:
            if (!g_ext) return -1;
            break;
        case 0x57 ... 0x58: return -1;
    }

    char c;
    if (g_shift)
        c = sc_shift_ascii[sc];
    else
        c = sc_ascii[sc];

    if (c >= 'a' && c <= 'z' && g_caps)
        c = (char)(c - 'a' + 'A');
    else if (c >= 'A' && c <= 'Z' && g_caps)
        c = (char)(c - 'A' + 'a');

    if (g_ctrl && c >= 'a' && c <= 'z')
        c = (char)(c - 'a' + 1);
    else if (g_ctrl && c >= 'A' && c <= 'Z')
        c = (char)(c - 'A' + 1);

    g_ext = false;
    return (int)(unsigned char)c;
}

bool kbd_data_ready(void)
{
    return (inb(KBD_STAT) & KBS_OBF) != 0;
}
