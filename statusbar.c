/* See COPYING for license terms */

#include "statusbar.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void sbe_show(struct sb_entry *sbe);
static void sbe_hide(struct sb_entry *sbe);

void sb_init(struct sb *sb, IxpClient *client) {
    INIT_LLIST_HEAD(&sb->sb_entries);
    sb->sb_client = client;
}

void sb_add(struct sb *sb, struct sb_entry *sbe) {
    llist_add(&sbe->sbe_list, &sb->sb_entries);
    sbe_init(sbe, sb);
}

void sb_update(struct sb *sb) {
    struct sb_entry *sbe;
    llist_for_each_entry(sbe, &sb->sb_entries, sbe_list) {
        sbe_update(sbe);
    }
}

void sb_finish(struct sb *sb) {
    struct sb_entry *sbe;
    llist_for_each_entry(sbe, &sb->sb_entries, sbe_list) {
        sbe_finish(sbe);
    }    
}

void sbe_init(struct sb_entry *sbe, struct sb *sb) {
    sbe->sbe_client = sb->sb_client;
    sbe->sbe_visible = 1;
    sbe->sbe_exists = 0;
    sbe_update(sbe);
}

void sbe_update(struct sb_entry *sbe) {
    int res;
    size_t sz;
    char contents[STATUSBAR_MAX_LABEL * 2]; // XXX

    if(sbe->sbe_visible) {
        sbe_show(sbe);

        if(sbe->sbe_update) {
            sbe->sbe_update(sbe);
        }
        
        sz = snprintf(contents, sizeof(contents),
                      "colors #%"PRIcolor" #%"PRIcolor" #%"PRIcolor"\nlabel %s\n",
                      sbe->sbe_foreground,
                      sbe->sbe_background,
                      sbe->sbe_border,
                      sbe->sbe_label);
        
        res = ixp_write(sbe->sbe_file, contents, sz);
        if(((size_t)res) != sz) {
            printf("ixp_write: %s\n", ixp_errbuf());
            abort();
        }
    } else {
        sbe_hide(sbe);
    }
}

void sbe_finish(struct sb_entry *sbe) {
    int res;
    sbe_hide(sbe);
    if(sbe->sbe_finish) {
        sbe->sbe_finish(sbe);
    }
}

void sbe_printf(struct sb_entry *sbe, const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    vsnprintf(sbe->sbe_label, sizeof(sbe->sbe_label), fmt, args);
    va_end(args);
}

void sbe_appendf(struct sb_entry *sbe, const char *fmt, ...) {
    size_t prevlen = strlen(sbe->sbe_label);
    va_list args;
    va_start(args, fmt);
    vsnprintf(sbe->sbe_label + prevlen, sizeof(sbe->sbe_label) - prevlen, fmt, args);
    va_end(args);
}

static void sbe_show(struct sb_entry *sbe) {
    if(!sbe->sbe_exists) {
        IxpCFid* file = ixp_create(sbe->sbe_client, sbe->sbe_path, 0777, P9_OWRITE);
        if(file == NULL) {
            printf("ixp_create: %s\n", ixp_errbuf());
            abort();
        }
        sbe->sbe_file = file;
        if(sbe->sbe_init) {
            sbe->sbe_init(sbe);
        }
        sbe->sbe_exists = 1;
    }
}

static void sbe_hide(struct sb_entry *sbe) {
    int res;
    if(sbe->sbe_file) {
        ixp_close(sbe->sbe_file);
        sbe->sbe_file = NULL;
    }
    if(sbe->sbe_exists) {
        res = ixp_remove(sbe->sbe_client, sbe->sbe_path);
        if(!res) {
            printf("ixp_remove: %s\n", ixp_errbuf());
        }
        sbe->sbe_exists = 0;
    }
}
