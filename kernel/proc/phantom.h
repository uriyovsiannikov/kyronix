#pragma once

#include "arch/x86_64/cpu.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

struct proc;

enum phantom_reason {
    PHANTOM_REASON_PAGE_FAULT = 1,
    PHANTOM_REASON_INVALID_OPCODE = 2,
    PHANTOM_REASON_PROTECTION = 3,
    PHANTOM_REASON_SECURITY = 4,
};

/* Move a suspicious execution into a private, simulated reality. */
bool phantom_intercept_fault(struct proc *p, cpu_state_t *state, uint64_t address);

/* Network and cryptographic persona providers used while diverted. */
bool phantom_fake_connect(struct proc *p, int fd, const void *address, size_t address_len);
bool phantom_dummy_crypto(struct proc *p, const char *purpose, void *out, size_t len);
