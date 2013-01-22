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
    if (o > 0) {
        args_index[argc] = start;
        argv[argc][o] = '\0';
        argc++;
    }
}

int cmd_suggest(int c, char *cmd, char *suggestions[]) {
    int count = 0;
    parse_cmd(cmd);
    if (argc <= 1) {
        int i;
        int len = strlen(cmd_argv(0));
        for (i = 0; i < cmd_count; i++) {
            if (!strncmp(cmd_argv(0), cmds[i].name, len))
                suggestions[count++] = cmds[i].name;
        }
        if (c >= 0) {
            for (i = CS_GAMECOMMANDS; i < CS_GAMECOMMANDS + MAX_GAMECOMMANDS; i++) {
                char *cs = cs_get(client_cs(c), i);
                if (!strncmp(cmd_argv(0), cs, len))
                    suggestions[count++] = cs;
            }
        }
    }
    return count;
}

void cmd_execute(int c, char *cmd) {
    parse_cmd(cmd);
    if (argc) {
        qboolean switch_screen = qfalse;
        int start = c;
        int end = c;
        int i;
        if (c < 0) {
            if (!strcmp(cmd_argv(0), "connect")) {
                for (i = 0; i < CLIENT_SCREENS; i++) {
                    if (!client_active(i)) {
                        start = i;
                        end = i;
                        switch_screen = qtrue;
                        break;
                    }
                }
                if (start < 0) {
                    ui_output(c, "No free screen left\n");
                    return;
                }
            } else {
                start = 0;
                end = CLIENT_SCREENS - 1;
            }
        }
        int executions = 0;
        for (client = start; client <= end; client++) {
            qboolean done = qfalse;
            for (i = 0; i < cmd_count; i++) {
                if (!strcmp(cmd_argv(0), cmds[i].name)) {
                    cmds[i].f();
                    if (switch_screen)
                        set_screen(client + 1);
                    done = qtrue;
                    break;
                }
            }
            if (!done) {
                for (i = CS_GAMECOMMANDS; i < CS_GAMECOMMANDS + MAX_GAMECOMMANDS; i++) {
                    if (!strcmp(cmd_argv(0), cs_get(client_cs(client), i))) {
                        client_command(c, "%s", cmd_args(0));
                        if (switch_screen)
                            set_screen(client + 1);
                        done = qtrue;
                        break;
                    }
                }
            }
            if (done)
                executions++;
            if (switch_screen)
                set_screen(client + 1);
        }
        if (executions == 0)
            ui_output(c, "Unrecognized command: %s\n", cmd);
        else if ((c < 0 && strcmp(cmd_argv(0), "connect")) || executions > 1)
            ui_output(c, "Command executed on %d clients\n", executions);
    }
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
