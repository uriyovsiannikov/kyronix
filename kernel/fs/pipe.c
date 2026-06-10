#include "pipe.h"
#include "lib/string.h"
#include "mm/heap.h"
#include "proc/proc.h"

#define EPIPE 32
#define EAGAIN 11

pipe_t* pipe_alloc(void)
{
    return (pipe_t*) kcalloc(1, sizeof(pipe_t));
}

void pipe_free(pipe_t* p)
{
    kfree(p);
}

int64_t pipe_read(pipe_t* p, void* buf, uint64_t len)
{
    uint8_t* out = (uint8_t*) buf;
    uint64_t done = 0;

    while (done < len)
    {
        if (p->count == 0)
        {
            if (p->write_refs == 0)
                break;
            p->waiting_reader = g_current_proc;
            sched_yield_blocking();
            p->waiting_reader = NULL;
            continue;
        }
        out[done++] = p->buf[p->rpos];
        p->rpos = (p->rpos + 1) % PIPE_BUFSZ;
        p->count--;
    }
    return (int64_t) done;
}

int64_t pipe_write(pipe_t* p, const void* buf, uint64_t len)
{
    if (p->read_refs == 0) {
        proc_send_signal(g_current_proc, SIGPIPE);
        return -(int64_t) EPIPE;
    }
    if (len == 0)
        return 0;

    const uint8_t* in = (const uint8_t*) buf;
    uint64_t done = 0;

    while (done < len && p->count < PIPE_BUFSZ)
    {
        uint32_t wpos = (p->rpos + p->count) % PIPE_BUFSZ;
        p->buf[wpos] = in[done++];
        p->count++;
    }

    /* wake any reader that was blocked on an empty pipe */
    if (done > 0 && p->waiting_reader)
    {
        proc_t* reader = (proc_t*) p->waiting_reader;
        if (reader->state == PROC_WAITING)
            reader->state = PROC_READY;
    }

    return (int64_t) done;
}
