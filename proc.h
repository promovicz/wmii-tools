/* See COPYING for license terms */

#ifndef TOOLS_PROC_H
#define TOOLS_PROC_H

#include <inttypes.h>

#define PROC_STAT_CPU_COUNT 10

struct proc_stat {
    union {
        uint64_t raw[PROC_STAT_CPU_COUNT];
        struct {
            uint64_t user;
            uint64_t nice;
            uint64_t system;
            uint64_t idle;
            uint64_t iowait;
            uint64_t irq;
            uint64_t softirq;
            uint64_t steal;
            uint64_t guest;
            uint64_t guest_nice;
        };
    } ps_cpu;
};

extern long proc_stat_unit();
extern int  proc_stat_read(struct proc_stat *ps);

extern void proc_stat_sub(struct proc_stat *out, struct proc_stat *a, struct proc_stat *b);


struct proc_meminfo {
    uint64_t mem_total;
    uint64_t mem_free;
    uint64_t mem_buffers;
    uint64_t mem_cached;
    uint64_t swap_free;
    uint64_t swap_total;
};

extern int proc_meminfo_read(struct proc_meminfo *mi);

#endif /* !TOOLS_PROC_H */
