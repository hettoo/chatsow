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
#include "ui.h"

#define MAX_CMDS 128
#define MAX_ARGC 512
#define MAX_ARG_SIZE 512

typedef struct cmd_s {
    char *name;
    void (*f)();
} cmd_t;
static cmd_t cmds[MAX_CMDS];
static int cmd_count = 0;

static int argc = 0;
static char argv[MAX_ARGC][MAX_ARG_SIZE];
static char *args;
static int args_index[MAX_ARGC];
static char args_stripped[MAX_ARGC * MAX_ARG_SIZE];
static int args_stripped_index[MAX_ARGC];

void parse_cmd() {
    int i;
    qboolean escaped = qfalse;
    char quote = '\0';
    int len = strlen(args);
    int o = 0;
    for (i = 0; i < len; i++) {
        switch (args[i]) {
            case ' ':
            case '\t':
            case '\n':
                if (o > 0) {
                    argc++;
                    o = 0;
                }
                break;
            case '\'':
            case '"':
                if (escaped) {
                } else if (quote == args[i]) {
                    quote = '\0';
                    argc++;
                    o = 0;
                    break;
                } else if (quote != '\0') {
                    quote = args[i];
                    break;
                }
            case '\\':
                if (args[i] == '\\') {
                    escaped = !escaped;
                    if (escaped)
                        break;
                }
            default:
                if (o < MAX_ARG_SIZE - 1)
                    argv[argc][o++] = args[i];
                break;
        }
    }
    if (o > 0)
        argc++;
}

void cmd_execute(char *cmd) {
    argc = 0;
    args = cmd;
    parse_cmd();
    int i;
    for (i = 0; i < cmd_count; i++) {
        if (!strcmp(cmd, cmds[i].name)) {
            cmds[i].f();
            return;
        }
    }
    ui_output("%s\n", cmd);
}

int cmd_argc() {
    return 0;
}

char *cmd_argv(int index) {
    if (index >= argc)
        return "";
    return "";
}

char *cmd_args(int start) {
    if (start >= argc)
        return "";
    return "";
}

char *cmd_args_stripped(int start) {
    if (start >= argc)
        return "";
    return "";
}

void cmd_add(char *name, void (*f)()) {
    cmds[cmd_count].name = name;
    cmds[cmd_count].f = f;
    cmd_count++;
}
