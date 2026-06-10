#pragma once
#include "mm/vmm.h"
#include <stdint.h>

/* push order in syscall_entry.S: rax last -> rax at rsp+0 */
typedef struct
{
    uint64_t rax; /* syscall nr on entry; return value on exit */
    uint64_t rbx;
    uint64_t rcx; /* user RIP (saved by SYSCALL) */
    uint64_t rdx; /* arg3 */
    uint64_t rsi; /* arg2 */
    uint64_t rdi; /* arg1 */
    uint64_t rbp;
    uint64_t r8;  /* arg5 */
    uint64_t r9;  /* arg6 */
    uint64_t r10; /* arg4 (linux: r10, not rcx) */
    uint64_t r11; /* user RFLAGS (saved by SYSCALL) */
    uint64_t r12;
    uint64_t r13;
    uint64_t r14;
    uint64_t r15;
} __attribute__((packed)) syscall_frame_t;

void syscall_dispatch(syscall_frame_t* f);

extern vmm_space_t* g_current_space;

void syscall_set_brk(uint64_t brk_base);
void signal_check(syscall_frame_t* f);
