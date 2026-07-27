#include "phantom.h"

#include "arch/x86_64/cpu.h"
#include "lib/log.h"
#include "lib/printf.h"
#include "lib/string.h"
#include "mm/pmm.h"
#include "mm/vma.h"
#include "mm/vmm.h"
#include "proc/jail.h"
#include "proc/proc.h"

/*
 * Phantom forks deliberately retain the original PID.  Replacing the address
 * space with a COW copy is the important boundary: observers and the attacker
 * see uninterrupted execution, while subsequent writes cannot affect the
 * pre-trigger reality.  The process is then moved into a full jail and all
 * external effects are served by persona providers below.
 */

static uint32_t phantom_seed(const proc_t *p, const char *purpose) {
    uint32_t h = 2166136261u ^ p->pid ^ ((uint32_t) p->phantom_generation << 16);
    for (const unsigned char *s = (const unsigned char *) purpose; s && *s; s++) {
        h ^= *s;
        h *= 16777619u;
    }
    return h;
}

static bool phantom_enter(proc_t *p, uint8_t reason, uint64_t address) {
    if (p->phantom_active) return true;

    vmm_space_t *isolated = vmm_space_new();
    if (!isolated) return false;
    if (vmm_fork_user(isolated, p->space) < 0) {
        vmm_space_free(isolated);
        return false;
    }

    kjail_conf_t conf;
    memset(&conf, 0, sizeof(conf));
    snprintf(conf.name, sizeof(conf.name), "phantom-%u", p->pid);
    snprintf(conf.root, sizeof(conf.root), "/.phantom/%u", p->pid);
    conf.flags = JAILF_ALL;
    conf.max_procs = 4;
    int jid = jail_create(JAIL_HOST, &conf, 0);
    if (jid < 0) {
        vmm_space_free(isolated);
        return false;
    }

    vmm_space_t *old = p->space;
    p->space = isolated;
    vmm_switch(isolated);
    vmm_space_free(old);
    jail_enter(p, (uint32_t) jid);
    p->phantom_active = 1;
    p->phantom_reason = reason;
    p->phantom_generation++;
    p->phantom_events = 1;
    p->phantom_last_address = address;
    log_warn("PHANTOM: diverted pid=%u reason=%u address=%lx jail=%d", p->pid, reason, address,
             jid);
    return true;
}

bool phantom_intercept_fault(struct proc *raw, cpu_state_t *raw_state, uint64_t address) {
    proc_t *p = (proc_t *) raw;
    cpu_state_t *state = (cpu_state_t *) raw_state;
    if (!p || !state || p->jail_exempt || address >= USER_LIMIT) return false;

    uint8_t reason = PHANTOM_REASON_PROTECTION;
    if (state->int_no == 14) reason = PHANTOM_REASON_PAGE_FAULT;
    if (state->int_no == 6) reason = PHANTOM_REASON_INVALID_OPCODE;
    if (state->int_no == 30) reason = PHANTOM_REASON_SECURITY;
    if (!phantom_enter(p, reason, address)) return false;

    p->phantom_events++;
    p->phantom_last_address = address;

    if (state->int_no == 14) {
        uint64_t page = address & ~(PAGE_SIZE - 1);
        void *decoy = pmm_alloc_zeroed();
        if (!decoy) return false;
        /* A read/write decoy page lets the probe continue without exposing data. */
        if (vmm_map(p->space, page, (uint64_t) decoy, VMM_UDATA) == 0) return true;
        pmm_free(decoy);
        return false;
    }

    /* Non-memory probes are consumed as a one-byte honeypot instruction. */
    state->rip++;
    return true;
}

bool phantom_fake_connect(struct proc *raw, int fd, const void *address, size_t address_len) {
    proc_t *p = (proc_t *) raw;
    if (!p || !p->phantom_active) return false;
    (void) fd;
    (void) address;
    (void) address_len;
    p->phantom_events++;
    log_info("PHANTOM: pid=%u synthetic network ACK", p->pid);
    return true;
}

bool phantom_dummy_crypto(struct proc *raw, const char *purpose, void *out, size_t len) {
    proc_t *p = (proc_t *) raw;
    if (!p || !p->phantom_active || !out) return false;
    uint32_t x = phantom_seed(p, purpose);
    unsigned char *dst = (unsigned char *) out;
    for (size_t i = 0; i < len; i++) {
        x ^= x << 13;
        x ^= x >> 17;
        x ^= x << 5;
        dst[i] = (unsigned char) x;
    }
    p->phantom_events++;
    return true;
}
