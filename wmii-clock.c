/* See COPYING for license terms */

    /*
    int res;
    struct timespec now;
    
    res = clock_gettime(CLOCK_MONOTONIC, &now);
    if(res == -1) {
        perror("clock_gettime");
        abort();
    }

    uint64_t delta = 0;
    if(now.tv_sec == s->ps_last.tv_sec) {
        delta += (now.tv_nsec - s->ps_last.tv_nsec) / 1000000;
    } else {
        long secdelta = now.tv_sec - s->ps_last.tv_sec;
        if(secdelta > 1) {
            delta += secdelta * 1000;
        }
        delta += ((1000000000 - s->ps_last.tv_nsec) + now.tv_nsec) / 1000000;
    }

    memcpy(&s->ps_last, &now, sizeof(s->ps_last));

    */

#define __STDC_FORMAT_MACROS

#include <inttypes.h>

#include <errno.h>
#include <time.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <sys/time.h>
#include <sys/epoll.h>
#include <sys/timerfd.h>

#include "colors.h"
#include "proc.h"
#include "signals.h"
#include "statusbar.h"

#define MAX_EVENTS 10


void update_clock(struct sb_entry *sbe) {
    int res;
    struct tm tm;
    struct timespec ts;

    res = clock_gettime(CLOCK_REALTIME, &ts);
    if(res == -1) {
        perror("clock_gettime");
        abort();
    }

    localtime_r(&ts.tv_sec, &tm);

    strftime(sbe->sbe_label, sizeof(sbe->sbe_label), "%Y-%m-%d - %H:%M:%S", &tm);
}

struct sb_cpu {
    long ps_unit;

    int ps_ready;

    struct timespec ps_last;

    struct proc_stat *ps_now;
    struct proc_stat *ps_old;

    struct proc_stat ps_a;
    struct proc_stat ps_b;
};

void init_cpu(struct sb_entry *sbe) {
    struct sb_cpu *s = malloc(sizeof(struct sb_cpu));
    if(s == NULL) {
        perror("malloc");
        abort();
    }

    sbe->sbe_private = (void*)s;

    s->ps_unit = proc_stat_unit();

    s->ps_now = &s->ps_a;
    s->ps_old = &s->ps_b;

    s->ps_ready = 0;
}

void update_cpu(struct sb_entry *sbe) {
    uint64_t total, idle, percent;
    struct proc_stat ps_delta;
    struct sb_cpu *s = (struct sb_cpu*)sbe->sbe_private;

    /* ensure we have an old value before actually running */
    if(!s->ps_ready) {
        if(!proc_stat_read(s->ps_old)) {
            s->ps_ready = 1;
        }
        return;
    }

    /* read cpu stats */
    if(proc_stat_read(s->ps_now)) {
        return;
    }
    
    /* compute the delta between the old and new stats */
    proc_stat_sub(&ps_delta, s->ps_now, s->ps_old);

    /* compute total time spent in delta */
    total = ps_delta.ps_cpu.idle
          + ps_delta.ps_cpu.nice
          + ps_delta.ps_cpu.user
          + ps_delta.ps_cpu.system;

    /* compute amount if idle time in delta */
    idle = s->ps_now->ps_cpu.idle - s->ps_old->ps_cpu.idle;

    /* scale up for precision */
    total *= 100;
    idle *= 100;

    /* compute non-idle percentage (actually in 1/10th percent) */
    percent = 1000 - (idle * 1000 / total);

    /* swap stat buffers */
    if(s->ps_now == &s->ps_a) {
        s->ps_now = &s->ps_b;
        s->ps_old = &s->ps_a;
    } else {
        s->ps_now = &s->ps_a;
        s->ps_old = &s->ps_b;
    }

    if(percent > 100) {
        sbe->sbe_background = COLOR_RED;
    } else {
        sbe->sbe_background = 0x444444;
    }

    /* print the status label */
    sbe_printf(sbe, "%.2"PRIu64".%1.1"PRIu64"%% busy",
               percent / 10, percent % 10);
}

void update_memory(struct sb_entry *sbe) {
    struct proc_meminfo mi;
    if(proc_meminfo_read(&mi)) {
        return;
    }

    uint64_t mused = mi.mem_total - mi.mem_free - mi.mem_buffers - mi.mem_cached;
    uint64_t mfree = mi.mem_free;
    uint64_t sused = mi.swap_total - mi.swap_free;
    uint64_t cused = mi.mem_cached;
    uint64_t bused = mi.mem_buffers;

    mused /= 1024;
    mfree /= 1024;
    sused /= 1024;
    cused /= 1024;
    bused /= 1024;

    if(sused > 0) {
        sbe_printf(sbe, "%"PRIu64"M used, %"PRIu64"M swap", mused, sused);
    } else {
        sbe_printf(sbe, "%"PRIu64"M used", mused);
    }
}


volatile int should_quit = 0;

void quit_handler(int sig) {
    should_quit = 1;
}

int
main(int argc, char **argv) {
    int n, nfds, res;
    void *sh;
    struct itimerspec its;
    struct epoll_event events[MAX_EVENTS];
    struct epoll_event timerevent;
    IxpClient* client;
    struct sb sb;

    signals_setup(&quit_handler);

    struct sb_entry sbe_memory = {
        .sbe_path = "/rbar/70_memory",
        .sbe_update = &update_memory,
        .sbe_foreground = 0xbbbbbb,
        .sbe_background = 0x444444,
        .sbe_border     = 0x555555,
    };

    struct sb_entry sbe_cpu = {
        .sbe_path = "/rbar/71_cpu",
        .sbe_init = &init_cpu,
        .sbe_update = &update_cpu,
        .sbe_foreground = 0xbbbbbb,
        .sbe_background = 0x444444,
        .sbe_border     = 0x555555,
    };

    struct sb_entry sbe_clock = {
        .sbe_path = "/rbar/90_clock",
        .sbe_update = &update_clock,
        .sbe_foreground = 0xbbbbbb,
        .sbe_background = 0x444444,
        .sbe_border     = 0x555555,
    };

    int epollfd = epoll_create1(EPOLL_CLOEXEC);
    if(epollfd == -1) {
        perror("epoll_create");
        abort();
    }

    int timerfd = timerfd_create(CLOCK_REALTIME, TFD_NONBLOCK|TFD_CLOEXEC);
    if(timerfd == -1) {
        perror("timerfd_create");
        abort();
    }
    timerevent.events = EPOLLIN;
    timerevent.data.fd = timerfd;
    its.it_interval.tv_sec = 0;
    its.it_interval.tv_nsec = 0;

    client = ixp_nsmount("wmii");
    if(client == NULL) {
        printf("ixp_nsmount: %s\n", ixp_errbuf());
        abort();
    }

    res = epoll_ctl(epollfd, EPOLL_CTL_ADD, timerfd, &timerevent);
    if(res == -1) {
        perror("epoll_ctl");
        abort();
    }

    sb_init(&sb, client);
    sb_add(&sb, &sbe_clock);
    sb_add(&sb, &sbe_memory);
    sb_add(&sb, &sbe_cpu);

    while(1) {
        res = clock_gettime(CLOCK_REALTIME, &its.it_value);
        if(res == -1) {
            perror("clock_gettime");
            abort();
        }

        its.it_value.tv_sec += 1;
        its.it_value.tv_nsec = 0;
        
        res = timerfd_settime(timerfd, TFD_TIMER_ABSTIME, &its, NULL);
        if(res == -1) {
            perror("timerfd_settime");
            abort();
        }

        nfds = epoll_wait(epollfd, events, MAX_EVENTS, -1);
        if(nfds == -1) {
		    if(errno != EINTR) {
                perror("epoll_wait");
                abort();
			}
        }

        if(should_quit) {
            break;
        }
        
        for (n = 0; n < nfds; n++) {
            if(events[n].data.fd == timerfd) {
                sb_update(&sb);
            }
        }
    }

    sb_finish(&sb);

    ixp_unmount(client);

    return 0;
}
