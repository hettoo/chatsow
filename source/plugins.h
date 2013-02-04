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

#ifndef WRLC_PLUGINS_H
#define WRLC_PLUGINS_H

#include "cs.h"

typedef struct plugin_interface_s {
    char *(*path)(char *format, ...);

    int (*ui_client)();
    void (*ui_output)(int client, char *format, ...);

    void (*client_say)(int id, char *format, ...);
    cs_t *(*client_cs)(int id);
    qboolean (*client_active)(int id);
    qboolean (*client_ready)(int id);
    int (*get_playernum)(int id);
    char *(*get_host)(int id);

    void (*cmd_execute)(int c, char *cmd);
    void (*cmd_execute_public)(int c, char *cmd);
    void (*cmd_execute_event)(int c, char *cmd);
    void (*cmd_execute_from_server)(int c, char *cmd);

    int (*cmd_client)();
    int (*cmd_argc)();
    char *(*cmd_argv)(int index);
    char *(*cmd_args)(int start);

    int (*cmd_add)(int client, char *name, void (*f)());
    int (*cmd_add_persistent)(int client, char *name, void (*f)());
    int (*cmd_add_generic)(char *name, void (*f)());
    int (*cmd_add_public)(int client, char *name, void (*f)());
    int (*cmd_add_public_persistent)(int client, char *name, void (*f)());
    int (*cmd_add_public_generic)(char *name, void (*f)());
    int (*cmd_add_event)(char *name, void (*f)());
    int (*cmd_add_from_server)(char *name, void (*f)());
    int (*cmd_add_server)(int client, char *name);
    int (*cmd_add_global)(char *name, void (*f)());
    int (*cmd_add_find_free)(char *name, void (*f)());
    int (*cmd_add_broadcast)(char *name, void (*f)());
    int (*cmd_add_broadcast_all)(char *name, void (*f)());

    void (*cmd_remove)(int index);
} plugin_interface_t;

#endif
