/* See COPYING for license terms */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sys/time.h>

#include <glib.h>
#include <upower.h>

#include "colors.h"
#include "signals.h"
#include "statusbar.h"

#define MAX_BATTERIES 2

GMainLoop *mainloop;

struct sb sb;

struct sb_entry sbe_ac = {
    .sbe_path = "/rbar/80_power_ac",
    .sbe_foreground = 0xbbbbbb,
    .sbe_background = 0x444444,
    .sbe_border     = 0x555555,
};

struct sb_entry sbe_batteries[MAX_BATTERIES] = {
    {
        .sbe_path = "/rbar/81_power_bat0",
        .sbe_foreground = 0xbbbbbb,
        .sbe_background = 0x444444,
        .sbe_border     = 0x555555,
    },
    {
        .sbe_path = "/rbar/82_power_bat1",
        .sbe_foreground = 0xbbbbbb,
        .sbe_background = 0x444444,
        .sbe_border     = 0x555555,
    },
};

static void
update_sb(UpClient *client)
{
    guint u;

    guint bat_count = 0;
    gboolean ac_online = FALSE;

    for(u = 0; u < MAX_BATTERIES; u++) {
        sbe_batteries[u].sbe_visible = 0;
    }

    GPtrArray *devices = up_client_get_devices(client);
    for(u = 0; u < devices->len; u++) {
        UpDevice *device = (UpDevice*)g_ptr_array_index(devices, u);

        UpDeviceKind kind;
        g_object_get(device,
                     "kind", &kind,
                     NULL);

        if(kind == UP_DEVICE_KIND_LINE_POWER) {
            gboolean online;

            g_object_get(device,
                         "online", &online,
                         NULL);

            sbe_printf(&sbe_ac, "AC %s", (online ? "on" : "off"));

            if(online) {
                ac_online = TRUE;
            }
        }
        if(kind == UP_DEVICE_KIND_BATTERY) {
            if(bat_count < MAX_BATTERIES) {
                struct sb_entry *sbe = &sbe_batteries[bat_count];

                gboolean bat_empty = FALSE;
                gboolean bat_charging = FALSE;
                gboolean bat_discharging = FALSE;

                guint state;
                double energy_rate;
                double energy_full;
                double energy_now;
                
                g_object_get(device,
                             "state", &state,
                             "energy", &energy_now,
                             "energy-full", &energy_full,
                             "energy-rate", &energy_rate,
                             NULL);
                
                const char *str;
                switch(state) {
                case UP_DEVICE_STATE_CHARGING:
                    if(energy_rate > 0.0) {
                        bat_charging = TRUE;
                    }
                    str = "chrg";
                    break;
                case UP_DEVICE_STATE_DISCHARGING:
                    if(energy_rate > 0.0) {
                        bat_discharging = TRUE;
                    }
                    str = "dsch";
                    break;
                case UP_DEVICE_STATE_EMPTY:
                    bat_empty = TRUE;
                    str = "empt";
                    break;
                case UP_DEVICE_STATE_FULLY_CHARGED:
                    str = "full";
                    break;
                case UP_DEVICE_STATE_PENDING_CHARGE:
                case UP_DEVICE_STATE_PENDING_DISCHARGE:
                    str = "idle";
                    break;
                default:
                    str = "unkn";
                    break;
                }
                
                double percent = (energy_now / energy_full) * 100.0;

                sbe_printf(sbe, "B%d %s %.1f%%", bat_count, str, percent);
                sbe->sbe_visible = 1;

                if(bat_empty) {
                    sbe->sbe_background = COLOR_RED;
                } else if(bat_charging) {
                    sbe->sbe_background = COLOR_YELLOW;
                } else if(bat_discharging) {
                    if(percent < 5.0) {
                        sbe->sbe_background = COLOR_RED;
                    } else if(percent < 15.0) {
                        sbe->sbe_background = COLOR_YELLOW;
                    } else {
                        sbe->sbe_background = COLOR_GREEN;
                    }
                } else {
                    sbe->sbe_background = 0x444444;
                }
                
                bat_count++;
            }
        }
    }

    if(ac_online) {
        sbe_ac.sbe_background = COLOR_GREEN;
    } else {
        sbe_ac.sbe_background = 0x444444;
    }

    sb_update(&sb);
}


static void device_added_cb (UpClient *client, UpDevice *device, gpointer user_data)
{
    up_client_enumerate_devices_sync(client, NULL, NULL);
    update_sb(client);
}

static void device_changed_cb (UpClient *client, UpDevice *device, gpointer user_data)
{
    up_client_enumerate_devices_sync(client, NULL, NULL);
    update_sb(client);
}

static void device_removed_cb (UpClient *client, UpDevice *device, gpointer user_data)
{
    up_client_enumerate_devices_sync(client, NULL, NULL);
    update_sb(client);
}

static void changed_cb (UpClient *client, gpointer user_data)
{
    update_sb(client);
}

void quit_handler(int sig) {
    g_main_loop_quit(mainloop);
}

int
main(int argc, char **argv) {
    int i;
    UpClient *upclient;
    IxpClient *client;

    signals_setup(&quit_handler);

    client = ixp_nsmount("wmii");
    if(client == NULL) {
        printf("ixp_nsmount: %s\n", ixp_errbuf());
        abort();
    }

    mainloop = g_main_loop_new(NULL, FALSE);

    upclient = up_client_new();

    sb_init(&sb, client);
    sb_add(&sb, &sbe_ac);    
    for(i = 0; i < MAX_BATTERIES; i++) {
        sb_add(&sb, &sbe_batteries[i]);
    }

    up_client_enumerate_devices_sync(upclient, NULL, NULL);

    g_signal_connect(upclient, "device-added", G_CALLBACK(device_added_cb), NULL);
    g_signal_connect(upclient, "device-removed", G_CALLBACK(device_removed_cb), NULL);
    g_signal_connect(upclient, "device-changed", G_CALLBACK(device_changed_cb), NULL);
    g_signal_connect(upclient, "changed", G_CALLBACK(changed_cb), NULL);

    update_sb(upclient);

    g_main_loop_run(mainloop);

    sb_finish(&sb);

    ixp_unmount(client);
}
