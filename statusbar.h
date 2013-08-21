/* See COPYING for license terms */

#ifndef TOOLS_STATUSBAR_H
#define TOOLS_STATUSBAR_H

#include "types.h"
#include "list.h"

#include <ixp.h>

#define STATUSBAR_MAX_LABEL 64

struct sb;
struct sb_entry;

typedef void (*sbe_init_cb_t)(struct sb_entry *sbe);
typedef void (*sbe_finish_cb_t)(struct sb_entry *sbe);
typedef void (*sbe_update_cb_t)(struct sb_entry *sbe);

struct sb {
    struct llist_head sb_entries;
    IxpClient*        sb_client;
};

extern void sb_init(struct sb *sb, IxpClient *client);
extern void sb_add(struct sb *sb, struct sb_entry *sbe);
extern void sb_update(struct sb *sb);
extern void sb_finish(struct sb *sb);

struct sb_entry {
    struct llist_head sbe_list;

    char*      sbe_path;
    IxpClient* sbe_client;
    IxpCFid*   sbe_file;

    int      sbe_visible;

    int      sbe_exists;
    int      sbe_changed;

    color_t  sbe_foreground;
    color_t  sbe_background;
    color_t  sbe_border;

    char     sbe_label[STATUSBAR_MAX_LABEL];

    void*    sbe_private;

    sbe_init_cb_t   sbe_init;
    sbe_finish_cb_t sbe_finish;
    sbe_update_cb_t sbe_update;
};

extern void sbe_init(struct sb_entry *sbe, struct sb *sb);
extern void sbe_update(struct sb_entry *sbe);
extern void sbe_finish(struct sb_entry *sbe);
extern void sbe_printf(struct sb_entry *sbe, const char *fmt, ...);
extern void sbe_appendf(struct sb_entry *sbe, const char *fmt, ...);

#endif /* !TOOLS_STATUSBAR_H */
