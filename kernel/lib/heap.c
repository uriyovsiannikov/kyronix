#include "heap.h"
#include "../arch/x86_64/pmm.h"
#include "../arch/x86_64/vmm.h"
#include "../lib/string.h"
#include "../lib/printf.h"

#define ALIGN_UP(v, a) (((v) + (a)-1) & ~((uint64_t)(a)-1))

typedef struct heap_block {
    uint64_t        size;
    struct heap_block *next, *prev;
} heap_block_t;

#define BLK_SIZE(s) ((s) & ~1ULL)
#define BLK_FREE(s) ((s) & 1ULL)
#define HDR_SZ      sizeof(heap_block_t)
#define MIN_BLK     (HDR_SZ + 16)

static heap_block_t *free_head;
static uint64_t      heap_start, heap_top;
static uint64_t      heap_max;

static void heap_free_block(heap_block_t *blk)
{
    blk->size |= 1;

    heap_block_t *cur = free_head, *prev = NULL;
    while (cur && cur < blk) { prev = cur; cur = cur->next; }

    if (prev) {
        uint8_t *end = (uint8_t *)prev + BLK_SIZE(prev->size);
        if ((heap_block_t *)end == blk) {
            prev->size = (BLK_SIZE(prev->size) + BLK_SIZE(blk->size)) | 1;
            blk = prev;
        }
    }

    if (cur) {
        uint8_t *end = (uint8_t *)blk + BLK_SIZE(blk->size);
        if ((heap_block_t *)end == cur) {
            blk->size = (BLK_SIZE(blk->size) + BLK_SIZE(cur->size)) | 1;
            heap_block_t *nxt = cur->next;
            if (cur->prev) cur->prev->next = nxt;
            else free_head = nxt;
            if (nxt) nxt->prev = cur->prev;
            cur = nxt;
        }
    }

    if (blk != prev) {
        blk->prev = prev;
        blk->next = cur;
        if (prev) prev->next = blk; else free_head = blk;
        if (cur) cur->prev = blk;
    }
}

static int heap_grow(void)
{
    if (heap_top >= heap_max) return -1;
    void *phys = pmm_alloc_page();
    if (!phys) return -1;
    if (vmm_map(heap_top, (uint64_t)phys, VMM_WRITABLE) < 0) {
        pmm_free_page(phys);
        return -1;
    }

    heap_block_t *blk = (heap_block_t *)(heap_top);
    blk->size = PAGE_SIZE;
    heap_top += PAGE_SIZE;
    heap_free_block(blk);
    return 0;
}

void heap_init(uint64_t start_virt)
{
    free_head = NULL;
    heap_start = heap_top = ALIGN_UP(start_virt, PAGE_SIZE);
    heap_max   = 0xFFFFFFFF90000000ULL;

    for (int i = 0; i < 16; i++) {
        if (heap_grow() < 0) {
            kprintf("heap: only %lu pages mapped\n", (unsigned long)i);
            break;
        }
    }
    kprintf("heap: start 0x%016lx\n", (unsigned long)heap_start);
}

void *kmalloc(size_t size)
{
    if (!size) size = 16;
    size_t need = ALIGN_UP(size + HDR_SZ, 16);
    if (need < MIN_BLK) need = MIN_BLK;

 retry:
    for (heap_block_t *blk = free_head; blk; blk = blk->next) {
        if (!BLK_FREE(blk->size)) continue;
        uint64_t bsz = BLK_SIZE(blk->size);
        if (bsz < need) continue;

        if (bsz >= need + MIN_BLK) {
            heap_block_t *newb = (heap_block_t *)((uint8_t *)blk + need);
            newb->size = (bsz - need) | 1;
            blk->size = need;

            newb->prev = blk;
            newb->next = blk->next;
            if (blk->next) blk->next->prev = newb;
            blk->next = newb;
        }

        blk->size &= ~1ULL;
        if (blk->prev) blk->prev->next = blk->next;
        else free_head = blk->next;
        if (blk->next) blk->next->prev = blk->prev;
        blk->next = blk->prev = NULL;

        memset(blk + 1, 0, BLK_SIZE(blk->size) - HDR_SZ);
        return (void *)(blk + 1);
    }

    if (heap_grow() == 0) goto retry;
    return NULL;
}

void kfree(void *ptr)
{
    if (!ptr) return;
    heap_block_t *blk = (heap_block_t *)ptr - 1;
    if (BLK_FREE(blk->size)) return;
    heap_free_block(blk);
}

void heap_print(void)
{
    for (heap_block_t *b = free_head; b; b = b->next)
        kprintf("  free 0x%016llx - 0x%016llx (%llu)\n",
                (unsigned long long)(uint64_t)b,
                (unsigned long long)((uint64_t)b + BLK_SIZE(b->size)),
                (unsigned long long)BLK_SIZE(b->size));
}
