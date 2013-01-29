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
#include <stdio.h>
#include <stdarg.h>
#include <dlfcn.h>

#include "main.h"
#include "import.h"
#include "plugins.h"
#include "cmd.h"
#include "ui.h"
#include "client.h"
#include "global.h"

static char base[512];

static plugin_interface_t trap;

void init(char *location) {
    strcpy(base, location);
    char *p;
    for (p = base + strlen(base) - 1; p >= base && *p != '/'; p--)
        ;
    p++;
    *p = '\0';

    trap.path = path;
    trap.ui_client = ui_client;
    trap.ui_output = ui_output;
    trap.client_cs = client_cs;
    trap.cmd_execute = cmd_execute;
    trap.cmd_client = cmd_client;
    trap.cmd_add_global = cmd_add_global;
    trap.cmd_add_generic = cmd_add_generic;
}

static void cmd_load() {
    void *handle;
    void (*loader)(plugin_interface_t *);

    char *name = cmd_argv(1);
    static char plugin_path[MAX_STRING_CHARS];
    strcpy(plugin_path, path("plugins/%s.so", name));

    handle = dlopen(plugin_path, RTLD_NOW);
    if (handle == NULL) {
        ui_output(cmd_client(), "Error loading %s: %s\n", name, dlerror());
        return;
    }
    loader = (void (*)(plugin_interface_t *))dlsym(handle, name);
    if (loader == NULL) {
        ui_output(cmd_client(), "Error finding function %s: %s\n", name, dlerror());
        return;
    }
    loader(&trap);
}

static void cmd_quit() {
    quit();
}

void register_general_commands() {
    cmd_add_global("load", cmd_load);
    cmd_add_global("quit", cmd_quit);
}

char *path(char *format, ...) {
    static char result[MAX_STRING_CHARS];
    strcpy(result, base);
    va_list argptr;
    va_start(argptr, format);
    vsprintf(result + strlen(base), format, argptr);
    va_end(argptr);
    return result;
}

int die(char *format, ...) {
    quit();
	va_list	argptr;
	va_start(argptr, format);
    vprintf(format, argptr);
	va_end(argptr);
    printf("\n");
    exit(1);
}
