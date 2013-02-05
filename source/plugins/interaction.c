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

#include "../plugins.h"
#include "../import.h"
#include "../utils.h"
#include "../cs.h"
#include "../ui.h"

plugin_interface_t *trap;

static int cmd_list_index;
static int cmd_find_index;
static int cmd_call_index;
static int cmd_server_index;

static void cmd_list() {
    static char message[MAX_MSGLEN];
    message[0] = '\0';
    int this = trap->cmd_client();
    int i;
    for (i = 0; i < CLIENTS; i++) {
        if (i != this && trap->client_ready(i)) {
            int j;
            for (j = 1; j <= MAX_CLIENTS; j++) {
                if (j != trap->get_playernum(i)) {
                    char *name = player_name(trap->client_cs(i), j);
                    if (name && *name) {
                        strcat(message, name);
                        strcat(message, "^7 ");
                    }
                }
            }
        }
    }
    trap->client_say(this, "%s", message);
}

static void cmd_find() {
    int i;
    char *server = NULL;
    char *ip;
    int port;
    char *map;
    char *real_name;
    for (i = 0; i < CLIENTS && !server; i++) {
        if (trap->client_ready(i)) {
            int j;
            for (j = 1; j <= MAX_CLIENTS && !server; j++) {
                char *name = player_name(trap->client_cs(i), j);
                if (name && partial_match(cmd_argv(1), name)) {
                    server = cs_get(trap->client_cs(i), 0);
                    ip = trap->get_host(i);
                    port = trap->get_port(i);
                    map = trap->get_level(i);
                    real_name = name;
                }
            }
        }
    }
    if (server)
        trap->client_say(trap->cmd_client(), "^7%s ^7is in %s ^7(%s:%d) [%s]", real_name, server, ip, port, map);
    else
        trap->client_say(trap->cmd_client(), "Not found");
}

static void cmd_call() {
    char *caller = player_name(trap->client_cs(trap->cmd_client()), trap->cmd_caller());
    int i;
    for (i = 0; i < CLIENTS; i++) {
        if (trap->client_ready(i)) {
            int j;
            for (j = 1; j <= MAX_CLIENTS; j++) {
                char *name = player_name(trap->client_cs(i), j);
                if (name && partial_match(cmd_argv(1), name)) {
                    char *server = cs_get(trap->client_cs(trap->cmd_client()), 0);
                    char *ip = trap->get_host(trap->cmd_client());
                    int port = trap->get_port(trap->cmd_client());
                    char *map = trap->get_level(trap->cmd_client());
                    trap->client_say(i, "^7%s^7: %s calls you from %s ^7(%s:%d) [%s]", name, caller, server, ip, port, map);
                    return;
                }
            }
        }
    }
    trap->client_say(trap->cmd_client(), "Not found");
}

static void cmd_server() {
    static char message[MAX_MSGLEN];
    message[0] = '\0';
    int this = trap->cmd_client();
    int i;
    qboolean first = qtrue;
    for (i = 0; i < CLIENTS; i++) {
        if (i != this && trap->client_ready(i)) {
            char *name = cs_get(trap->client_cs(i), 0);
            if (name && *name) {
                if (!first)
                    strcat(message, "^7 || ");
                else
                    first = qfalse;
                strcat(message, name);
            }
        }
    }
    trap->client_say(this, "%s", message);
}

void init(plugin_interface_t *new_trap) {
    trap = new_trap;
    cmd_list_index = trap->cmd_add_public_generic("list", cmd_list);
    cmd_find_index = trap->cmd_add_public_generic("find", cmd_find);
    cmd_call_index = trap->cmd_add_public_generic("call", cmd_call);
    cmd_server_index = trap->cmd_add_public_generic("server", cmd_server);
}

void frame() {
}

void shutdown() {
    trap->cmd_remove(cmd_list_index);
    trap->cmd_remove(cmd_find_index);
    trap->cmd_remove(cmd_call_index);
    trap->cmd_remove(cmd_server_index);
}
