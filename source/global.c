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
#include <unistd.h>
#include <sys/types.h>
#include <pwd.h>

#include "main.h"
#include "import.h"
#include "plugins.h"
#include "cmd.h"
#include "ui.h"
#include "client.h"
#include "global.h"

#define MAX_PLUGINS 64

typedef struct plugin_s {
    char name[MAX_STRING_CHARS];
    void *handle;
    void (*init)(plugin_interface_t *);
    void (*frame)();
    void (*shutdown)();
} plugin_t;

plugin_t plugins[MAX_PLUGINS];
int plugin_count = 0;

static char base[512];
static char home_base[512];

static plugin_interface_t trap;

void init(char *location) {
    strcpy(base, location);
    char *p;
    for (p = base + strlen(base) - 1; p >= base && *p != '/'; p--)
        ;
    p++;
    *p = '\0';

    struct passwd *pw = getpwuid(getuid());
    strcpy(home_base, pw->pw_dir);
    strcat(home_base, "/.chatsow/");

    trap.path = path;

    trap.ui_client = ui_client;
    trap.ui_output = ui_output;

    trap.client_cs = client_cs;

    trap.cmd_execute = cmd_execute;
    trap.cmd_execute_special = cmd_execute_special;

    trap.cmd_client = cmd_client;
    trap.cmd_argc = cmd_argc;
    trap.cmd_argv = cmd_argv;
    trap.cmd_args = cmd_args;

    trap.cmd_add = cmd_add;
    trap.cmd_add_persistent = cmd_add_persistent;
    trap.cmd_add_generic = cmd_add_generic;
    trap.cmd_add_special = cmd_add_special;
    trap.cmd_add_special_persistent = cmd_add_special_persistent;
    trap.cmd_add_special_generic = cmd_add_special_generic;
    trap.cmd_add_from_server = cmd_add_from_server;
    trap.cmd_add_server = cmd_add_server;
    trap.cmd_add_global = cmd_add_global;
    trap.cmd_add_find_free = cmd_add_find_free;
    trap.cmd_add_broadcast = cmd_add_broadcast;
    trap.cmd_add_broadcast_all = cmd_add_broadcast_all;
}

static void cmd_exec() {
    char current[MAX_STRING_CHARS];

    FILE *fp = fopen(path("%s", cmd_argv(1)), "r");
    if (!fp)
        return;

    while (fgets(current, MAX_STRING_CHARS, fp) > 0) {
        if (current[0] != '\n')
            cmd_execute(cmd_client(), current);
    }

    fclose(fp);
}

static void cmd_load() {
    if (plugin_count == MAX_PLUGINS) {
        ui_output(cmd_client(), "Too many plugins\n");
        return;
    }
    int i;
    for (i = 0; i < plugin_count; i++) {
        if (!strcmp(plugins[i].name, cmd_argv(1))) {
            ui_output(cmd_client(), "%s is already loaded\n", plugins[i].name);
            return;
        }
    }

    plugin_t *plugin = plugins + plugin_count++;
    strcpy(plugin->name, cmd_argv(1));
    static char plugin_path[MAX_STRING_CHARS];
    strcpy(plugin_path, path("plugins/%s.so", plugin->name));

    plugin->handle = dlopen(plugin_path, RTLD_NOW);
    if (plugin->handle == NULL) {
        ui_output(cmd_client(), "Error loading %s: %s\n", plugin->name, dlerror());
        plugin_count--;
        return;
    }
    plugin->init = (void (*)(plugin_interface_t *))dlsym(plugin->handle, "init");
    if (plugin->init == NULL) {
        ui_output(cmd_client(), "Error finding function init: %s\n", dlerror());
        dlclose(plugin->handle);
        plugin_count--;
        return;
    }
    plugin->frame = (void (*)())dlsym(plugin->handle, "frame");
    if (plugin->frame == NULL) {
        ui_output(cmd_client(), "Error finding function frame: %s\n", dlerror());
        dlclose(plugin->handle);
        plugin_count--;
        return;
    }
    plugin->shutdown = (void (*)())dlsym(plugin->handle, "shutdown");
    if (plugin->shutdown == NULL) {
        ui_output(cmd_client(), "Error finding function shutdown: %s\n", dlerror());
        dlclose(plugin->handle);
        plugin_count--;
        return;
    }
    plugin->init(&trap);
}

static void cmd_unload() {
    int i;
    int skip = 0;
    for (i = 0; i < plugin_count - skip; i++) {
        if (!strcmp(plugins[i].name, cmd_argv(1))) {
            plugins[i].shutdown();
            dlclose(plugins[i].handle);
            skip++;
        }
        if (i < plugin_count - skip)
            plugins[i] = plugins[i + skip];
    }
    plugin_count -= skip;
    if (skip == 0)
        ui_output(-2, "Plugin %s not loaded\n", cmd_argv(1));
}

static void cmd_plugins() {
    ui_output(-2, "^5Loaded plugins:\n");
    int i;
    for (i = 0; i < plugin_count; i++) {
        ui_output(-2, "%s\n", plugins[i].name);
    }
}

void plugin_frame() {
    int i;
    for (i = 0; i < plugin_count; i++)
        plugins[i].frame();
}

void plugin_shutdown() {
    int i;
    for (i = 0; i < plugin_count; i++) {
        plugins[i].shutdown();
        dlclose(plugins[i].handle);
    }
}

static void cmd_quit() {
    quit();
}

void register_general_commands() {
    cmd_add_global("exec", cmd_exec);
    cmd_add_global("load", cmd_load);
    cmd_add_global("unload", cmd_unload);
    cmd_add_global("plugins", cmd_plugins);
    cmd_add_global("quit", cmd_quit);
}

char *path(char *format, ...) {
    static char result[MAX_STRING_CHARS];
    strcpy(result, home_base);
    va_list argptr;
    va_start(argptr, format);
    vsprintf(result + strlen(home_base), format, argptr);
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
