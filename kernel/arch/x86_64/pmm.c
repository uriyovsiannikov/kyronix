#include "pmm.h"
#include "../lib/string.h"
#include "../lib/printf.h"

#define PMM_MAX_FRAMES   (4 * 1024 * 1024)
#define PMM_BITMAP_BYTES (PMM_MAX_FRAMES / 8)

static uint8_t  pmm_bitmap[PMM_BITMAP_BYTES];
static size_t   pmm_last_alloc;

uint64_t g_hhdm_offset;
size_t   g_total_frames;
size_t   g_free_frames;

static void bm_set(size_t b) { pmm_bitmap[b / 8] |=   (1 << (b % 8)); }
static void bm_clr(size_t b) { pmm_bitmap[b / 8] &= ~(1 << (b % 8)); }
static int  bm_tst(size_t b) { return (pmm_bitmap[b / 8] >> (b % 8)) & 1; }

static void mark_used(uint64_t phys, size_t len)
{
    uint64_t sf = (phys + PAGE_SIZE - 1) >> PAGE_SHIFT;
    uint64_t ef = (phys + len) >> PAGE_SHIFT;
    if (ef > g_total_frames) ef = g_total_frames;
    for (uint64_t f = sf; f < ef; f++)
        if (!bm_tst(f)) { bm_set(f); g_free_frames--; }
}

static void mark_free(uint64_t phys, size_t len)
{
    uint64_t sf = (phys + PAGE_SIZE - 1) >> PAGE_SHIFT;
    uint64_t ef = (phys + len) >> PAGE_SHIFT;
    if (ef > g_total_frames) ef = g_total_frames;
    for (uint64_t f = sf; f < ef; f++)
        if (bm_tst(f)) { bm_clr(f); g_free_frames++; }
}

void pmm_init(uint64_t hhdm,
              struct limine_memmap_response *mmap,
              uint64_t kernel_phys, size_t kernel_size)
{
    g_hhdm_offset  = hhdm;
    pmm_last_alloc = 0;

    uint64_t max_addr = 0;
    for (uint64_t i = 0; i < mmap->entry_count; i++) {
        struct limine_memmap_entry *e = mmap->entries[i];
        if (e->type != LIMINE_MEMMAP_USABLE &&
            e->type != LIMINE_MEMMAP_BOOTLOADER_RECLAIMABLE)
            continue;
        uint64_t end = e->base + e->length;
        if (end > max_addr) max_addr = end;
    }

    g_total_frames = max_addr >> PAGE_SHIFT;
    if (g_total_frames > PMM_MAX_FRAMES) g_total_frames = PMM_MAX_FRAMES;

    memset(pmm_bitmap, 0xFF, g_total_frames / 8);
    g_free_frames = 0;

    for (uint64_t i = 0; i < mmap->entry_count; i++) {
        struct limine_memmap_entry *e = mmap->entries[i];
        if (e->type == LIMINE_MEMMAP_USABLE ||
            e->type == LIMINE_MEMMAP_BOOTLOADER_RECLAIMABLE)
            mark_free(e->base, e->length);
    }

    mark_used(kernel_phys, kernel_size);
    mark_used(0, PAGE_SIZE);

    kprintf("pmm: %lu frames, %lu free (%lu MiB)\n",
            (unsigned long)g_total_frames,
            (unsigned long)g_free_frames,
            (unsigned long)((g_free_frames * PAGE_SIZE) / (1024 * 1024)));
}

void *pmm_alloc_page(void)
{
    for (size_t i = pmm_last_alloc; i < g_total_frames; i++)
        if (!bm_tst(i)) { bm_set(i); pmm_last_alloc = i; g_free_frames--; return (void *)(i << PAGE_SHIFT); }
    for (size_t i = 0; i < pmm_last_alloc; i++)
        if (!bm_tst(i)) { bm_set(i); pmm_last_alloc = i; g_free_frames--; return (void *)(i << PAGE_SHIFT); }
    return NULL;
}

void pmm_free_page(void *phys)
{
    uint64_t addr = (uint64_t)phys;
    if (addr & (PAGE_SIZE - 1)) return;
    size_t idx = addr >> PAGE_SHIFT;
    if (idx >= g_total_frames) return;
    bm_clr(idx);
    g_free_frames++;
}
