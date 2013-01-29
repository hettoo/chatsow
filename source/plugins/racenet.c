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

plugin_interface_t *trap;

static void cmd_racenet() {
    FILE *fp = popen(trap->path("plugins/racenet.pl wrace1"), "r");
    int c;
    while ((c = fgetc(fp)) != EOF)
        trap->ui_output(-2, "%c", c);
}

void racenet(plugin_interface_t *new_trap) {
    trap = new_trap;
    trap->cmd_add_global("racenet", cmd_racenet);
}
