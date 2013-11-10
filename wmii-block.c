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
#include "sysfs.h"
#include "signals.h"
#include "statusbar.h"

#define MAX_EVENTS 10

struct sb_block {
    char *devname;
    int bs_ready;
    struct sysfs_block_stat *bs_now;
    struct sysfs_block_stat *bs_old;
    struct sysfs_block_stat bs_a;
    struct sysfs_block_stat bs_b;
};

void init_block(struct sb_entry *sbe) {
    char *devname = (char*)sbe->sbe_private;
    struct sb_block *s = malloc(sizeof(struct sb_block));
    if(s == NULL) {
        perror("malloc");
        abort();
    }

    s->devname = devname;

    s->bs_now = &s->bs_a;
    s->bs_old = &s->bs_b;

    s->bs_ready = 0;

    sbe->sbe_private = (void*)s;

    sbe_printf(sbe, "%s", devname);
}

void update_block(struct sb_entry *sbe) {
    struct sb_block *b = (struct sb_block*)sbe->sbe_private;

    uint64_t nr = 0, nw = 0;

    /* ensure we have an old value before actually running */
    if(!b->bs_ready) {
        if(!sysfs_read_block_stat(b->devname, b->bs_old)) {
            b->bs_ready = 1;
        }
        return;
    }

    /* read block stats */
    if(sysfs_read_block_stat(b->devname, b->bs_now)) {
        return;
    }

    /* compute stat difference */
    nr = b->bs_now->read_processed - b->bs_old->read_processed;
    nw = b->bs_now->write_processed - b->bs_old->write_processed;

    /* swap stat buffers */
    if(b->bs_now == &b->bs_a) {
        b->bs_now = &b->bs_b;
        b->bs_old = &b->bs_a;
    } else {
        b->bs_now = &b->bs_a;
        b->bs_old = &b->bs_b;
    }

    /* set color according to stats */
    if (nw > 0) {
        sbe->sbe_background = COLOR_RED;
    } else if (nr > 0) {
        sbe->sbe_background = COLOR_GREEN;
    } else {
        sbe->sbe_background = 0x444444;
    }

    /* update the sbe */
    sbe_printf(sbe, "%s", b->devname);
}

volatile int should_quit = 0;

void quit_handler(int sig) {
    should_quit = 1;
}

int
main(int argc, char **argv) {
    int n, nfds, res;
    struct itimerspec timerits;
    struct epoll_event events[MAX_EVENTS];
    struct epoll_event timerevent;
    IxpClient* client;
    struct sb sb;

    signals_setup(&quit_handler);

    struct sb_entry sbe_sda = {
        .sbe_path = "/rbar/60_sda",
        .sbe_private = "sda",
        .sbe_init = &init_block,
        .sbe_update = &update_block,
        .sbe_foreground = 0xbbbbbb,
        .sbe_background = 0x444444,
        .sbe_border     = 0x555555,        
    };

    struct sb_entry sbe_sdb = {
        .sbe_path = "/rbar/61_sdb",
        .sbe_private = "sdb",
        .sbe_init = &init_block,
        .sbe_update = &update_block,
        .sbe_foreground = 0xbbbbbb,
        .sbe_background = 0x444444,
        .sbe_border     = 0x555555,        
    };

    struct sb_entry sbe_sdc = {
        .sbe_path = "/rbar/62_sdc",
        .sbe_private = "sdc",
        .sbe_init = &init_block,
        .sbe_update = &update_block,
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
    timerits.it_interval.tv_sec = 0;
    timerits.it_interval.tv_nsec = 250 * 1000 * 1000;
    timerits.it_value.tv_sec = timerits.it_interval.tv_sec;
    timerits.it_value.tv_nsec = timerits.it_interval.tv_nsec;

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
        
    res = timerfd_settime(timerfd, 0, &timerits, NULL);
    if(res == -1) {
        perror("timerfd_settime");
        abort();
    }

    sb_init(&sb, client);
    sb_add(&sb, &sbe_sda);
    sb_add(&sb, &sbe_sdb);
    sb_add(&sb, &sbe_sdc);

    while(1) {
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
                uint64_t x;
                read(timerfd, &x, sizeof(x));
                sb_update(&sb);
            }
        }
    }

    sb_finish(&sb);

    ixp_unmount(client);

    return 0;
}
