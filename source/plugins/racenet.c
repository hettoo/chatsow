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

#include <stdio.h>

#include "../plugins.h"
#include "../import.h"
#include "../cs.h"

plugin_interface_t *trap;

static void cmd_racenet() {
    FILE *fp = popen(trap->path("plugins/racenet.pl %s", cs_get(trap->client_cs(trap->ui_client()), CS_MAPNAME)), "r");
    static char cmd[MAX_STRING_CHARS];
    strcpy(cmd, "say \"");
    int i = strlen(cmd);
    int c;
    while ((c = fgetc(fp)) != EOF)
        cmd[i++] = (char)c;
    if (cmd[i - 1] == '\n')
        i--;
    cmd[i++] = '"';
    cmd[i++] = '\0';
    trap->cmd_execute(trap->cmd_client(), cmd);
}

void racenet(plugin_interface_t *new_trap) {
    trap = new_trap;
    trap->cmd_add_generic("racenet", cmd_racenet);
}
