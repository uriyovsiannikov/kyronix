#include "vmm.h"
#include "pmm.h"
#include "cpu.h"
#include "../lib/string.h"
#include "../lib/printf.h"

typedef uint64_t pte_t;

#define PML4_SHIFT 39
#define PDPT_SHIFT 30
#define PD_SHIFT   21
#define PT_SHIFT   12

#define PML4_MASK  0x1FF
#define PDPT_MASK  0x1FF
#define PD_MASK    0x1FF
#define PT_MASK    0x1FF

#define PTE_ADDR_MASK 0x000FFFFFFFFF000ULL
#define PTE_FLAGS_MASK 0xFFF
#define PTE_PS (1ULL << 7)

static pte_t *pml4_virt;

static pte_t *walk_page(uint64_t virt, int alloc)
{
    size_t idx;

    idx = (virt >> PML4_SHIFT) & PML4_MASK;
    pte_t *pml4e = &pml4_virt[idx];
    uint64_t pml4_phys = *pml4e & PTE_ADDR_MASK;

    if (!pml4_phys) {
        if (!alloc) return NULL;
        pml4_phys = (uint64_t)pmm_alloc_page();
        if (!pml4_phys) return NULL;
        pte_t *pdpt_virt = (pte_t *)(g_hhdm_offset + pml4_phys);
        memset(pdpt_virt, 0, PAGE_SIZE);
        *pml4e = pml4_phys | VMM_PRESENT | VMM_WRITABLE;
    }

    idx = (virt >> PDPT_SHIFT) & PDPT_MASK;
    pte_t *pdpte = &((pte_t *)(g_hhdm_offset + pml4_phys))[idx];
    if (*pdpte & PTE_PS) return NULL;
    uint64_t pdpt_phys = *pdpte & PTE_ADDR_MASK;

    if (!pdpt_phys) {
        if (!alloc) return NULL;
        pdpt_phys = (uint64_t)pmm_alloc_page();
        if (!pdpt_phys) return NULL;
        pte_t *pd_virt = (pte_t *)(g_hhdm_offset + pdpt_phys);
        memset(pd_virt, 0, PAGE_SIZE);
        *pdpte = pdpt_phys | VMM_PRESENT | VMM_WRITABLE;
    }

    idx = (virt >> PD_SHIFT) & PD_MASK;
    pte_t *pde = &((pte_t *)(g_hhdm_offset + pdpt_phys))[idx];
    if (*pde & PTE_PS) return NULL;
    uint64_t pd_phys = *pde & PTE_ADDR_MASK;

    if (!pd_phys) {
        if (!alloc) return NULL;
        pd_phys = (uint64_t)pmm_alloc_page();
        if (!pd_phys) return NULL;
        pte_t *pt_virt = (pte_t *)(g_hhdm_offset + pd_phys);
        memset(pt_virt, 0, PAGE_SIZE);
        *pde = pd_phys | VMM_PRESENT | VMM_WRITABLE;
    }

    idx = (virt >> PT_SHIFT) & PT_MASK;
    return &((pte_t *)(g_hhdm_offset + pd_phys))[idx];
}

void vmm_init(void)
{
    uint64_t boot_cr3;
    __asm__ __volatile__("mov %%cr3, %0" : "=r"(boot_cr3));

    pte_t *boot_pml4 = (pte_t *)(g_hhdm_offset + boot_cr3);

    uint64_t new_pml4_phys = (uint64_t)pmm_alloc_page();
    if (!new_pml4_phys) {
        kprintf("vmm: failed to allocate PML4\n");
        return;
    }
    pml4_virt = (pte_t *)(g_hhdm_offset + new_pml4_phys);

    memset(pml4_virt, 0, PAGE_SIZE);

    for (int i = 256; i < 512; i++)
        pml4_virt[i] = boot_pml4[i];

    __asm__ __volatile__("mov %0, %%cr3" :: "r"(new_pml4_phys) : "memory");

    kprintf("vmm: new PML4 at phys 0x%016lx\n", (unsigned long)new_pml4_phys);
}

int vmm_map(uint64_t virt, uint64_t phys, uint64_t flags)
{
    pte_t *pte = walk_page(virt, 1);
    if (!pte) return -1;

    uint64_t entry = (phys & PTE_ADDR_MASK) | flags | VMM_PRESENT;
    *pte = entry;

    __asm__ __volatile__("invlpg (%0)" :: "r"(virt) : "memory");
    return 0;
}

void vmm_unmap(uint64_t virt)
{
    pte_t *pte = walk_page(virt, 0);
    if (!pte) return;

    *pte = 0;
    __asm__ __volatile__("invlpg (%0)" :: "r"(virt) : "memory");
}

uint64_t vmm_translate(uint64_t virt)
{
    pte_t *pte = walk_page(virt, 0);
    if (!pte) return 0;

    uint64_t phys = *pte & PTE_ADDR_MASK;
    if (!phys) return 0;

    uint64_t offset = virt & 0xFFF;
    return phys | offset;
}
