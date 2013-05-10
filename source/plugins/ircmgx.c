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
#include "../cs.h"
#include "../utils.h"

#define IRC_QUERY_TIMEOUT 8000

plugin_interface_t *trap;

static int mirror_index;
static int ircrecv_index;

static unsigned int query;
static char target[64];

static char message[4096];

static void cmd_mirror() {
    char *text = trap->cmd_argv(1);
    if (partial_match("made a new MGX record", text)) {
        int c = trap->cmd_client();
        cs_t *cs = trap->client_cs(c);
        sprintf(message, "[%s @ %s] %s", trap->get_level(c), cs_get(cs, CS_HOSTNAME), text);
        trap->irc_say(NULL, "%s\n", uncolor(message));
    }
    if (query) {
        if (partial_match("No highscores found yet!", text)) {
            trap->irc_say(target, "%s\n", uncolor(text));
            query = 0;
        }
        if (partial_match("Top 3 players on", text)) {
            char *result = uncolor(text);
            result = strchr(result, '\n') + 1;
            char *end = strchr(result, '\n');
            if (end)
                *end = '\0';
            trap->irc_say(target, "%s\n", result);
            query = 0;
        }
    }
}

static void cmd_ircrecv() {
    if (!strcmp(trap->cmd_argv(2), "!record")) {
        if (!query) {
            if (trap->cmd_argc() < 4) {
                trap->irc_say(trap->cmd_argv(1), "Usage: %s mapname\n", trap->cmd_argv(2));
            } else {
                strcpy(message, "top nopj 3 ");
                strcat(message, trap->cmd_argv(3));
                trap->cmd_execute(0, message);
                query = millis();
                strcpy(target, trap->cmd_argv(1));
            }
        } else {
            trap->irc_say(trap->cmd_argv(1), "A query is already running\n");
        }
    }
}

void init(plugin_interface_t *new_trap) {
    trap = new_trap;

    query = 0;

    mirror_index = trap->cmd_add_from_server("pr", cmd_mirror);
    ircrecv_index = trap->cmd_add_event("ircrecv", cmd_ircrecv);
}

void frame() {
    if (query && millis() > query + IRC_QUERY_TIMEOUT) {
        trap->irc_say(target, "Query timed out.\n");
        query = 0;
    }
}

void shutdown() {
    trap->cmd_remove(mirror_index);
    trap->cmd_remove(ircrecv_index);
}
