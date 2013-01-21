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
#include "cs.h"
#include "client.h"
#include "ui.h"
#include "cmd.h"

#define MAX_CMDS 128
#define MAX_ARGC 512
#define MAX_ARG_SIZE 512
#define MAX_ARGS_SIZE (MAX_ARGC * MAX_ARG_SIZE)

typedef struct cmd_s {
    char *name;
    void (*f)();
} cmd_t;
static cmd_t cmds[MAX_CMDS];
static int cmd_count = 0;

static int client = -1;
static int argc = 0;
static char argv[MAX_ARGC][MAX_ARG_SIZE];
static char args[MAX_ARGS_SIZE];
static int args_index[MAX_ARGC];

void parse_cmd() {
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
    if (o > 0) {
        args_index[argc] = start;
        argv[argc][o] = '\0';
        argc++;
    }
}

void cmd_execute(int c, char *cmd) {
    client = c;
    argc = 0;
    strcpy(args, cmd);
    parse_cmd();
    if (argc) {
        int i;
        for (i = 0; i < cmd_count; i++) {
            if (!strcmp(cmd_argv(0), cmds[i].name)) {
                cmds[i].f();
                return;
            }
        }
        if (c >= 0) {
            for (i = CS_GAMECOMMANDS; i < CS_GAMECOMMANDS + MAX_GAMECOMMANDS; i++) {
                if (!strcmp(cmd_argv(0), cs_get(client_cs(c), i))) {
                    client_command(c, "%s", cmd_args(0));
                    return;
                }
            }
        }
    }
    ui_output(c, "Unrecognized command: %s\n", cmd);
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

void cmd_add(char *name, void (*f)()) {
    cmds[cmd_count].name = name;
    cmds[cmd_count].f = f;
    cmd_count++;
}
