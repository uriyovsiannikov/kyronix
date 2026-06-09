#pragma once

#include <stdint.h>
#include <stddef.h>
#include "../boot/limine.h"

#define PAGE_SIZE  4096
#define PAGE_SHIFT 12

extern uint64_t g_hhdm_offset;
extern size_t   g_total_frames;
extern size_t   g_free_frames;

void  pmm_init(uint64_t hhdm,
               struct limine_memmap_response *mmap,
               uint64_t kernel_phys_base, size_t kernel_size);
void *pmm_alloc_page(void);
void  pmm_free_page(void *phys);
