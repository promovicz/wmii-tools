#ifndef TOOLS_SYSFS_H
#define TOOLS_SYSFS_H

#include <stdint.h>

#define SYSFS_BLOCK_STAT_COUNT 11

struct sysfs_block_stat {
    union {
        uint64_t raw[SYSFS_BLOCK_STAT_COUNT];
        struct {
            uint64_t read_processed;
            uint64_t read_merges;
            uint64_t read_sectors;
            uint64_t read_ticks;
            uint64_t write_processed;
            uint64_t write_merges;
            uint64_t write_sectors;
            uint64_t write_ticks;
            uint64_t in_flight;
            uint64_t io_ticks;
            uint64_t time_in_queue;
        };
    };
};

extern int sysfs_read_ints(char *str, uint64_t *out, int count);

extern int sysfs_read_block_stat(char *devname, struct sysfs_block_stat *out);

#endif /* !TOOLS_SYSFS_H */
