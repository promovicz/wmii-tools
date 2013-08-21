/* See COPYING for license terms */

#include "proc.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

long proc_stat_unit() {
    return sysconf(_SC_CLK_TCK);
}

void proc_stat_sub(struct proc_stat *out, struct proc_stat *a, struct proc_stat *b) {
    int i;
    for(i = 0; i < PROC_STAT_CPU_COUNT; i++) {
        out->ps_cpu.raw[i] = a->ps_cpu.raw[i] - b->ps_cpu.raw[i];
    }
}

static void proc_stat_read_n(char *str, uint64_t *out, int count) {
    char *value = str;
    for(int i = 0; i < count; i++) {
        char *nvalue;
        uint64_t v = strtoull(value, &nvalue, 10);
        if(nvalue == value || nvalue == NULL) {
            break;
        }
        value = nvalue;
        out[i] = v;
    }
}

int proc_stat_read(struct proc_stat *ps) {
    int ret = 0;
    FILE *fp;
    char buf[256];
    memset((void*)ps, 0, sizeof(*ps));
    fp = fopen("/proc/stat", "r");
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
        char *value = strchr(str, ' ');
        if(!value) {
            ret = 1;
            break;
        }
        *(value++) = 0;
        if(!strcmp(name, "cpu")) {
            proc_stat_read_n(value, ps->ps_cpu.raw, PROC_STAT_CPU_COUNT);
        }
    }
    fclose(fp);
    return ret;
}
