#include "proc.h"
#include "arch/x86_64/syscall_setup.h"
#include "lib/log.h"
#include "lib/string.h"
#include "mm/heap.h"
#include "mm/pmm.h"
#include "syscall/syscall.h" /* g_current_space */

proc_t g_proctable[PROC_MAX];
proc_t* g_current_proc = NULL;

void proc_init(void)
{
    memset(g_proctable, 0, sizeof(g_proctable));
    log_info("PROC: table initialised  (%d slots)", PROC_MAX);
}

proc_t* proc_alloc(uint32_t ppid)
{
    for (int i = 0; i < PROC_MAX; i++)
    {
        if (g_proctable[i].state != PROC_UNUSED)
            continue;

        proc_t* p = &g_proctable[i];
        memset(p, 0, sizeof(*p));

        p->state = PROC_READY;
        p->pid = (uint32_t) (i + 1);
        p->ppid = ppid;
        p->pgid = (uint32_t) (i + 1);
        p->wait_for = 0;

        p->kstack = (uint8_t*) kmalloc(KSTACK_SIZE);
        if (!p->kstack)
        {
            p->state = PROC_UNUSED;
            return NULL;
        }
        p->kstack_top = (uint64_t) (p->kstack + KSTACK_SIZE);

        p->fds = (vfs_file_t**) kcalloc(VFS_FD_MAX, sizeof(vfs_file_t*));
        if (!p->fds)
        {
            kfree(p->kstack);
            p->state = PROC_UNUSED;
            return NULL;
        }

        p->mmap_bump = 0x0000500000000000ULL;
        p->cwd[0] = '/';
        p->cwd[1] = '\0';
        return p;
    }
    return NULL;
}

proc_t* proc_find(uint32_t pid)
{
    for (int i = 0; i < PROC_MAX; i++)
        if (g_proctable[i].state != PROC_UNUSED && g_proctable[i].pid == pid)
            return &g_proctable[i];
    return NULL;
}

proc_t* proc_next_ready(proc_t* skip)
{
    for (int i = 0; i < PROC_MAX; i++)
    {
        if (&g_proctable[i] == skip)
            continue;
        if (g_proctable[i].state == PROC_READY)
            return &g_proctable[i];
    }
    return NULL;
}

void sched_yield_blocking(void)
{
    proc_t* p = g_current_proc;
    proc_t* next = proc_next_ready(p);
    if (!next)
        return;

    p->state = PROC_WAITING;
    next->state = PROC_RUNNING;
    vfs_set_fdtable(next->fds);
    g_current_space = next->space;
    cpu_set_kernel_stack(next->kstack_top);
    sched_switch(next);

    p->state = PROC_RUNNING;
    vfs_set_fdtable(p->fds);
    g_current_space = p->space;
    cpu_set_kernel_stack(p->kstack_top);
}
