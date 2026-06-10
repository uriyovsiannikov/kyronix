#include "elf.h"
#include "lib/log.h"
#include "lib/string.h"
#include "mm/pmm.h"
#include "mm/vmm.h"

int elf_load(const void* data, uint64_t size, elf_load_result_t* out)
{
    if (!data || size < sizeof(Elf64_Ehdr))
    {
        log_error("ELF: too small");
        return -1;
    }

    const Elf64_Ehdr* ehdr = (const Elf64_Ehdr*) data;

    if (ehdr->e_ident[EI_MAG0] != ELFMAG0 || ehdr->e_ident[EI_MAG1] != ELFMAG1 ||
        ehdr->e_ident[EI_MAG2] != ELFMAG2 || ehdr->e_ident[EI_MAG3] != ELFMAG3)
    {
        log_error("ELF: bad magic");
        return -1;
    }
    if (ehdr->e_ident[EI_CLASS] != ELFCLASS64)
    {
        log_error("ELF: not 64-bit");
        return -1;
    }
    if (ehdr->e_ident[EI_DATA] != ELFDATA2LSB)
    {
        log_error("ELF: not little-endian");
        return -1;
    }
    if (ehdr->e_type != ET_EXEC && ehdr->e_type != ET_DYN)
    {
        log_error("ELF: not executable");
        return -1;
    }
    if (ehdr->e_machine != EM_X86_64)
    {
        log_error("ELF: not x86_64");
        return -1;
    }
    if (ehdr->e_phentsize < sizeof(Elf64_Phdr) || ehdr->e_phnum == 0)
    {
        log_error("ELF: no program headers");
        return -1;
    }

    vmm_space_t* space = vmm_space_new();
    if (!space)
    {
        log_error("ELF: OOM (space)");
        return -1;
    }

    uint64_t brk = 0;
    uint64_t phdr_va_out = 0;

    for (uint16_t i = 0; i < ehdr->e_phnum; i++)
    {
        const Elf64_Phdr* ph = (const Elf64_Phdr*) ((const uint8_t*) data + ehdr->e_phoff +
                                                    (uint64_t) i * ehdr->e_phentsize);

        if (ph->p_type == PT_LOAD && phdr_va_out == 0)
        {
            /* phdr VA = p_vaddr + (e_phoff - p_offset) when phdrs are in first segment */
            if (ehdr->e_phoff >= ph->p_offset && ehdr->e_phoff < ph->p_offset + ph->p_filesz)
            {
                phdr_va_out = ph->p_vaddr + (ehdr->e_phoff - ph->p_offset);
            }
        }

        if (ph->p_type != PT_LOAD)
            continue;
        if (ph->p_memsz == 0)
            continue;

        if (ph->p_offset + ph->p_filesz > size)
        {
            log_error("ELF: segment out of file bounds");
            vmm_space_free(space);
            return -1;
        }

        uint64_t vflags = VMM_PRESENT | VMM_USER;
        if (ph->p_flags & PF_W)
            vflags |= VMM_WRITE | VMM_NX;
        if (!(ph->p_flags & PF_X))
            vflags |= VMM_NX;

        uint64_t page_base = PAGE_ALIGN_DOWN(ph->p_vaddr);
        uint64_t page_end = PAGE_ALIGN_UP(ph->p_vaddr + ph->p_memsz);

        for (uint64_t pg = page_base; pg < page_end; pg += PAGE_SIZE)
        {
            void* phys = pmm_alloc_zeroed();
            if (!phys)
            {
                log_error("ELF: OOM loading segment");
                vmm_space_free(space);
                return -1;
            }

            if (vmm_map(space, pg, (uint64_t) phys, vflags) < 0)
            {
                log_error("ELF: vmm_map failed");
                pmm_free(phys);
                vmm_space_free(space);
                return -1;
            }

            uint64_t file_start_va = ph->p_vaddr;
            uint64_t file_end_va = ph->p_vaddr + ph->p_filesz;

            uint64_t copy_va_start = (pg > file_start_va) ? pg : file_start_va;
            uint64_t copy_va_end = (pg + PAGE_SIZE < file_end_va) ? pg + PAGE_SIZE : file_end_va;

            if (copy_va_start < copy_va_end)
            {
                uint64_t dst_off = copy_va_start - pg;
                uint64_t src_off = ph->p_offset + (copy_va_start - file_start_va);
                uint64_t n = copy_va_end - copy_va_start;
                memcpy((uint8_t*) phys_to_virt((uint64_t) phys) + dst_off,
                       (const uint8_t*) data + src_off, n);
            }
        }

        /* track highest virtual address for brk */
        uint64_t seg_end = PAGE_ALIGN_UP(ph->p_vaddr + ph->p_memsz);
        if (seg_end > brk)
            brk = seg_end;
    }

    out->space = space;
    out->entry = ehdr->e_entry;
    out->brk = brk;
    out->phdr_va = phdr_va_out;
    out->phentsize = ehdr->e_phentsize;
    out->phnum = ehdr->e_phnum;

    log_info("ELF: loaded  entry=0x%lx  brk=0x%lx  phdr_va=0x%lx", out->entry, out->brk,
             out->phdr_va);
    return 0;
}
