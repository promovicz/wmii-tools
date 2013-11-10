
#include <stdio.h>
#include <stdlib.h>

#include "signals.h"

void signals_setup(sighandler_t handler) {
    void *sh;
    sh = signal(SIGINT, handler);
    if(sh == SIG_ERR) {
        perror("signal");
        abort();
    }
    sh = signal(SIGABRT, handler);
    if(sh == SIG_ERR) {
        perror("signal");
        abort();
    }
    sh = signal(SIGTERM, handler);
    if(sh == SIG_ERR) {
        perror("signal");
        abort();
    }
}
