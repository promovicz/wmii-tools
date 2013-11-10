#ifndef TOOLS_SIGNALS_H
#define TOOLS_SIGNALS_H

#include <signal.h>

/* XXX glibc */
typedef __sighandler_t sighandler_t;

extern void signals_setup(sighandler_t handler);

#endif /* !TOOLS_SIGNALS_H */
