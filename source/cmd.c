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

#include "import.h"
#include "utils.h"
#include "cs.h"
#include "client.h"
#include "cmd.h"

typedef enum cmd_type_e {
    CT_NORMAL,
    CT_FROM_SERVER,
    CT_SERVER,
    CT_GLOBAL,
    CT_FIND_FREE,
    CT_BROADCAST,
    CT_BROADCAST_ALL,
    CT_TOTAL
} cmd_type_t;

typedef struct cmd_s {
    char *name;
    void (*f)();
    cmd_type_t type;
    qboolean clients[CLIENTS];
} cmd_t;

static cmd_t cmds[MAX_CMDS];
static int cmd_count = 0;

static int client = -1;
static int argc = 0;
static char argv[MAX_ARGC][MAX_ARG_SIZE];
static char args[MAX_ARGS_SIZE];
static int args_index[MAX_ARGC];

void parse_cmd(char *cmd) {
    argc = 0;
    strcpy(args, cmd);
    int i;
    qboolean escaped = qfalse;
    char quote = '\0';
    int len = strlen(args);
    int o = 0;
    int start = 0;
    for (i = 0; i < len; i++) {
        qboolean normal = qfalse;
        qboolean skip = qfalse;
        char add = '\0';
        if (escaped) {
            if (quote != args[i] && args[i] != '\\')
                add = '\\';
            escaped = qfalse;
            normal = qtrue;
        } else {
            switch (args[i]) {
                case '\\':
                    escaped = qtrue;
                    break;
                case ' ':
                case '\t':
                case '\n':
                    if (quote != '\0')
                        normal = qtrue;
                    else if (o > 0)
                        skip = qtrue;
                    else
                        start = i + 1;
                    break;
                case '\'':
                case '"':
                    if (quote == args[i])
                        skip = qtrue;
                    else if (quote == '\0')
                        quote = args[i];
                    else
                        normal = qtrue;
                    break;
                default:
                    normal = qtrue;
                    break;
            }
        }
        if (skip) {
            args_index[argc] = start;
            argv[argc][o] = '\0';
            argc++;
            o = 0;
            start = i + 1;
        }
        if (add != '\0' && o < MAX_ARG_SIZE - 1)
            argv[argc][o++] = add;
        if (normal && o < MAX_ARG_SIZE - 1)
            argv[argc][o++] = args[i];
    }
    if (o > 0 || (i >= 1 && (args[i - 1] == ' ' || args[i - 1] == '\t' || args[i - 1] == '\n'))) {
        args_index[argc] = start;
        argv[argc][o] = '\0';
        argc++;
    }
}

static qboolean cmd_type_extends(int type, int parent) {
    if (type == parent)
        return qtrue;

    if (parent == CT_NORMAL && type == CT_SERVER)
        return qtrue;

    if (parent == CT_GLOBAL && (type == CT_FIND_FREE || type == CT_BROADCAST || type == CT_BROADCAST_ALL))
        return qtrue;

    return qfalse;
}

static qboolean cmd_type_compatible(int type, int parent) {
    if (cmd_type_extends(type, parent))
        return qtrue;

    if (parent == CT_NORMAL && cmd_type_extends(type, CT_GLOBAL))
        return qtrue;

    return qfalse;
}

static cmd_t *cmd_find(cmd_t *cmd, int c, int type, qboolean partial) {
    if (!partial && cmd_argc() == 0)
        return NULL;

    int i;
    int len = strlen(cmd_argv(0));
    for (i = cmd ? (cmd - cmds) + 1 : 0; i < cmd_count; i++) {
        if (cmd_type_compatible(cmds[i].type, type)
                && (partial ? !strncmp(cmds[i].name, cmd_argv(0), len)
                    : !strcmp(cmds[i].name, cmd_argv(0)))
                && (c < 0 || cmds[i].clients[c]))
            return cmds + i;
    }

    return NULL;
}

static int normal_type(int c) {
    return c >= 0 ? CT_NORMAL : CT_GLOBAL;
}

int cmd_suggest(int c, char *name, char suggestions[][MAX_SUGGESTION_SIZE]) {
    int type = normal_type(c);
    int count = 0;
    parse_cmd(name);
    if (argc <= 1) {
        cmd_t *cmd = NULL;
        while ((cmd = cmd_find(cmd, c, type, qtrue)) != NULL) {
            if (cmd->name[0])
                strcpy(suggestions[count++], cmd->name);
        }
    }
    return count;
}

static void cmd_execute_real(int c, char *name, int type) {
    parse_cmd(name);

    cmd_t *cmd = cmd_find(NULL, c, type, qfalse);
    if (!cmd) {
        ui_output(c, "Unknown command: \"%s\"\n", cmd_argv(0));
        return;
    }

    int start = c;
    int end = c;
    qboolean switch_screen = qfalse;
    if (cmd_type_compatible(cmd->type, type))
        type = cmd->type;
    if (type == CT_BROADCAST) {
        start = 0;
        end = CLIENTS - 1;
    } else if (type == CT_BROADCAST_ALL) {
        start = -1;
        end = CLIENTS - 1;
    } else if (type == CT_FIND_FREE && c < 0) {
        int i;
        qboolean found = qfalse;
        for (i = 0; i < CLIENTS && !found; i++) {
            if (!client_active(i)) {
                start = i;
                end = i;
                found = qtrue;
                switch_screen = qtrue;
                break;
            }
        }
        if (!found) {
            ui_output(c, "No free client found.\n");
            return;
        }
    }
    int executions = 0;
    for (client = start; client <= end; client++) {
        if (type == CT_SERVER)
            client_command(client, "%s", cmd_args(0));
        else
            cmd->f();
        if (switch_screen)
            set_screen(client + 1);
    }
    if (type == CT_BROADCAST)
        ui_output(c, "Command executed on %d clients.\n", executions);
}

void cmd_execute(int c, char *cmd) {
    cmd_execute_real(c, cmd, normal_type(c));
}

void cmd_execute_from_server(int c, char *cmd) {
    cmd_execute_real(c, cmd, CT_FROM_SERVER);
}

int cmd_client() {
    return client;
}

int cmd_argc() {
    return argc;
}

char *cmd_argv(int index) {
    if (index >= argc)
        return "";
    return argv[index];
}

char *cmd_args(int start) {
    if (start >= argc)
        return "";
    return args + args_index[start];
}

static cmd_t *cmd_find_exact(char *name, void(*f)(), int type) {
    int i;
    for (i = 0; i < cmd_count; i++) {
        if (cmds[i].f == f && cmds[i].name == name && cmds[i].type == type)
            return cmds + i;
    }
    return NULL;
}

static cmd_t *cmd_reserve(char *name, void (*f)(), int type) {
    cmd_t *cmd = cmd_find_exact(name, f, type);
    if (cmd)
        return cmd;

    if (cmd_count == MAX_CMDS)
        die("Command count overflow.\n");
    cmd = cmds + cmd_count++;

    int i;
    for (i = 0; i < CLIENTS; i++)
        cmd->clients[i] = qfalse;
    cmd->name = name;
    cmd->f = f;
    cmd->type = type;

    return cmd;
}

static void cmd_allow_all(cmd_t *cmd) {
    int i;
    for (i = 0; i < CLIENTS; i++)
        cmd->clients[i] = qtrue;
}

void cmd_add(int client, char *name, void (*f)()) {
    cmd_t *cmd = cmd_reserve(name, f, CT_NORMAL);
    cmd->clients[client] = qtrue;
}

void cmd_add_from_server(char *name, void (*f)()) {
    cmd_t *cmd = cmd_reserve(name, f, CT_FROM_SERVER);
    cmd_allow_all(cmd);
}

void cmd_add_server(int client, char *name) {
    cmd_t *cmd = cmd_reserve(name, NULL, CT_SERVER);
    cmd->clients[client] = qtrue;
}

void cmd_add_global(char *name, void (*f)()) {
    cmd_t *cmd = cmd_reserve(name, f, CT_GLOBAL);
    cmd_allow_all(cmd);
}

void cmd_add_find_free(char *name, void (*f)()) {
    cmd_t *cmd = cmd_reserve(name, f, CT_FIND_FREE);
    cmd_allow_all(cmd);
}

void cmd_add_broadcast(char *name, void (*f)()) {
    cmd_t *cmd = cmd_reserve(name, f, CT_BROADCAST);
    cmd_allow_all(cmd);
}

void cmd_add_broadcast_all(char *name, void (*f)()) {
    cmd_t *cmd = cmd_reserve(name, f, CT_BROADCAST_ALL);
    cmd_allow_all(cmd);
}
