/* See COPYING for license terms */

#include "proc.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int proc_meminfo_read(struct proc_meminfo *mi) {
    int ret = 0;
    FILE *fp;
    char buf[256];
    memset((void*)mi, 0, sizeof(*mi));
    fp = fopen("/proc/meminfo", "r");
    if(fp == NULL) {
        perror("fopen");
        abort();
    }
    while(1) {
        char *str = fgets(buf, sizeof(buf), fp);
        if(!str) {
            break;
        }
        char *name = str;
        char *value = strrchr(str, ':');
        if(!value) {
            ret = 1;
            break;
        }
        *(value++) = 0;
        unsigned long long v = strtoull(value, NULL, 10);
        if(!strcmp(name, "MemTotal")) {
            mi->mem_total = v;
        }
        if(!strcmp(name, "MemFree")) {
            mi->mem_free = v;
        }
        if(!strcmp(name, "Buffers")) {
            mi->mem_buffers = v;
        }
        if(!strcmp(name, "Cached")) {
            mi->mem_cached = v;
        }
        if(!strcmp(name, "SwapTotal")) {
            mi->swap_total = v;
        }
        if(!strcmp(name, "SwapFree")) {
            mi->swap_free = v;
        }
    }
    fclose(fp);
    return ret;
}
