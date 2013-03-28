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

plugin_interface_t *trap;

static int cmd_index;

static int test = 0;

static int indices[CLIENTS][MAX_CLIENTS];

static void cmd_external() {
    int target_bytes = atoi(trap->cmd_argv(1));
    int target_bits = target_bytes * 8;
    if (!strcmp(trap->cmd_argv(2 + target_bytes), "dstart")) {
        int i;
        for (i = 0; i < target_bits; i++) {
            if ((atoi(trap->cmd_argv(2 + (i >> 3))) & (1 << (i & 7)))) {
                FILE *fp = fopen(trap->path("demos/runs/%d_%d_%d.wd%d", trap->cmd_client(), i, test, PROTOCOL), "w");
                trap->ui_output(cmd_client(), "Start recording for %d\n", i);
                indices[trap->cmd_client()][i] = trap->client_record(trap->cmd_client(), fp, i);
                test++;
            }
        }
    }
    if (!strcmp(trap->cmd_argv(2 + target_bytes), "dcancel") || !strcmp(trap->cmd_argv(2 + target_bytes), "dcancel")) {
        int i;
        for (i = 0; i < target_bits; i++) {
            if ((atoi(trap->cmd_argv(2 + (i >> 3))) & (1 << (i & 7))) && indices[trap->cmd_client()][i] != -1)
                trap->ui_output(cmd_client(), "Stop recording for %d\n", i);
                trap->client_stop_record(trap->cmd_client(), indices[trap->cmd_client()][i]);
                indices[trap->cmd_client()][i] = -1;
        }
    }
}

void init(plugin_interface_t *new_trap) {
    trap = new_trap;
    cmd_index = trap->cmd_add_event("external", cmd_external);
    int i;
    for (i = 0; i < CLIENTS; i++) {
        int j;
        for (j = 0; j < MAX_CLIENTS; j++)
            indices[i][j] = -1;
    }
}

void frame() {
}

void shutdown() {
    trap->cmd_remove(cmd_index);
}
