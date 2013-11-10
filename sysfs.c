
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>

#include "sysfs.h"

int sysfs_read_ints(char *str, uint64_t *out, int count) {
    int i, c = 0;
    char *value = str;
    for(i = 0; i < count; i++) {
        char *nvalue;
        uint64_t v = strtoull(value, &nvalue, 10);
        if(nvalue == value || nvalue == NULL) {
            break;
        }
        c++;
        value = nvalue;
        out[i] = v;
    }
    return c;
}

int sysfs_read_block_stat(char *devname, struct sysfs_block_stat *out) {
    int ret = 0;
    FILE *fp;
    char path[PATH_MAX];
    char buf[8192];
    memset((void*)out, 0, sizeof(*out));
    snprintf(path, sizeof(path), "/sys/block/%s/stat", devname);
    fp = fopen(path, "r");
    if(fp == NULL) {
        perror("fopen");
        abort();
    }
    char *str = fgets(buf, sizeof(buf), fp);
    if(!str) {
        printf("Inavalid block stat for %s\n", devname);
        return -1;
    }
    fclose(fp);
    ret = sysfs_read_ints(buf, out->raw, SYSFS_BLOCK_STAT_COUNT);
    if(ret != SYSFS_BLOCK_STAT_COUNT) {
        printf("Bad number of values for %s\n", devname);
        return -1;
    }
    return 0;
}
