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
#include "../utils.h"

plugin_interface_t *trap;

static int cmd_index;

static void cmd_help() {
    char *text = trap->cmd_argv(2);
    if (partial_match("?", text)) {
        if (partial_match("how", text) && (partial_match("start", text) || partial_match("join", text)))
            trap->client_say(trap->cmd_client(), "Press escape and choose join to start");
        if ((partial_match("how", text) || partial_match("help", text))
                && (partial_match("run", text) || partial_match("jump", text)
                    || partial_match("move", text) || partial_match("fast", text) || partial_match("speed", text))) {
            trap->client_say(trap->cmd_client(),
                    "Open the console (below escape) and type cg_oldmovement 1\n"
                    "^2Go to mgxrace.com to download a strafehud");
            trap->client_say(trap->cmd_client(),
                    "To learn racing: spectate someone, Google strafe jumping or find a Warsow movement tutorial video");
        }
    }
}

void init(plugin_interface_t *new_trap) {
    trap = new_trap;
    cmd_index = trap->cmd_add_from_server("ch", cmd_help);
}

void frame() {
}

void shutdown() {
    trap->cmd_remove(cmd_index);
}
