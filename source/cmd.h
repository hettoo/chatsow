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

#ifndef WRLC_CMD_H
#define WRLC_CMD_H

#define MAX_CMDS 128
#define MAX_ARGC 512
#define MAX_ARG_SIZE 512
#define MAX_ARGS_SIZE (MAX_ARGC * MAX_ARG_SIZE)

#include "ui.h"

void cmd_execute(int c, char *cmd);
int cmd_suggest(int c, char *cmd, char suggestions[][MAX_SUGGESTION_SIZE]);
int cmd_client();
int cmd_argc();
char *cmd_argv(int index);
char *cmd_args(int start);

void cmd_add(char *name, void (*f)());

#endif
