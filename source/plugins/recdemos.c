/*
Copyright (C) 2013 hettoo (Gerco van Heerdt)

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/

#include <stdlib.h>

#include "../plugins.h"
#include "../client.h"
#include "../utils.h"

#define RECBUFFER 3
#define POSTRUN_TIME 8000

typedef struct recdemo_s {
    int id;
    qboolean stopped;
    qboolean record;
    signed int stop_time;
} recdemo_t;

typedef struct manager_s {
    recdemo_t demos[RECBUFFER];
    int current;
} manager_t;

plugin_interface_t *trap;

static int cmd_index;

static manager_t demos[CLIENTS][MAX_CLIENTS];

static void stop(int c, int t, int d) {
    trap->client_stop_record(c, demos[c][t].demos[d].id);
    demos[c][t].demos[d].id = -1;
}

static void schedule_stop(int c, int t) {
    manager_t *manager = &demos[c][t];
    if (manager->current == -1)
        return;
    recdemo_t *demo = manager->demos + manager->current;
    if (demo->id == -1 || demo->stopped)
        return;
    demo->stopped = qtrue;
    demo->stop_time = millis();
}

static void start(int c, int t) {
    manager_t *manager = &demos[c][t];
    schedule_stop(c, t);
    int i;
    recdemo_t *demo = NULL;
    for (i = 0; i < RECBUFFER; i++) {
        if (manager->demos[i].id == -1) {
            manager->current = i;
            demo = manager->demos + i;
            break;
        }
    }
    if (demo == NULL) {
        manager->current = (manager->current + 1) % RECBUFFER;
        demo = manager->demos + manager->current;
    }
    demo->stopped = qfalse;
    FILE *fp = fopen(trap->path("demos/runs/%d_%d_%d.wd%d", c, t, manager->current, PROTOCOL), "w");
    demo->id = trap->client_record(trap->cmd_client(), fp, t);
}

void frame() {
    signed int time = millis();
    int i;
    for (i = 0; i < CLIENTS; i++) {
        int j;
        for (j = 0; j < MAX_CLIENTS; j++) {
            manager_t *manager = &demos[i][j];
            int k;
            for (k = 0; k < RECBUFFER; k++) {
                recdemo_t *demo = manager->demos + k;
                if (demo->id >= 0 && demo->stopped && time >= demo->stop_time + POSTRUN_TIME)
                    stop(i, j, k);
            }
        }
    }
}

static void cmd_external() {
    int target_bytes = atoi(trap->cmd_argv(1));
    int target_bits = target_bytes * 8;
    if (!strcmp(trap->cmd_argv(2 + target_bytes), "dstart")) {
        int i;
        for (i = 0; i < target_bits; i++) {
            if ((atoi(trap->cmd_argv(2 + (i >> 3))) & (1 << (i & 7))))
                start(trap->cmd_client(), i);
        }
    }
}

void init(plugin_interface_t *new_trap) {
    trap = new_trap;
    cmd_index = trap->cmd_add_event("external", cmd_external);
    int i;
    for (i = 0; i < CLIENTS; i++) {
        int j;
        for (j = 0; j < MAX_CLIENTS; j++) {
            manager_t *manager = &demos[i][j];
            manager->current = -1;
            int k;
            for (k = 0; k < RECBUFFER; k++)
                manager->demos[k].id = -1;
        }
    }
}

void shutdown() {
    int i;
    for (i = 0; i < CLIENTS; i++) {
        int j;
        for (j = 0; j < MAX_CLIENTS; j++) {
            manager_t *manager = &demos[i][j];
            int k;
            for (k = 0; k < RECBUFFER; k++) {
                if (manager->demos[k].id != -1)
                    stop(i, j, k);
            }
        }
    }
    trap->cmd_remove(cmd_index);
}
