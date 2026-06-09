#pragma once

#include <stdint.h>
#include <stddef.h>

#define VMM_PRESENT     (1ULL << 0)
#define VMM_WRITABLE    (1ULL << 1)
#define VMM_USER        (1ULL << 2)
#define VMM_NX          (1ULL << 63)

void  vmm_init(void);
int   vmm_map(uint64_t virt, uint64_t phys, uint64_t flags);
void  vmm_unmap(uint64_t virt);
uint64_t vmm_translate(uint64_t virt);
