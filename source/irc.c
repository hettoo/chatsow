/*
Copyright (C) 1997-2001 Id Software, Inc.

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
#include <stdlib.h>

#include "net.h"
#include "ui.h"
#include "irc.h"

static sock_t sock;

static char *nick;
static char *channel;
static char *host;
static int port;

static void irc_send(char *format, ...) {
    va_list argptr;
    va_start(argptr, format);
    msg_t *msg = sock_init_send_raw(&sock);
    vwrite_string(msg, format, argptr);
    va_end(argptr);
    msg->cursize--;
    write_string(msg, "\r\n");
    msg->cursize--;
    sock_send(&sock);
}

void irc_say(char *to, char *format, ...) {
    static char string[MAX_MSGLEN];
    va_list argptr;
    va_start(argptr, format);
    vsprintf(string, format, argptr);
    va_end(argptr);
    irc_send("PRIVMSG %s :%s", to == NULL ? channel : to, string);
}

static void irc_connect() {
    sock_connect_tcp(&sock, host, port);
    irc_send("NICK %s", nick);
    irc_send("USER %s 0 * :%s", nick, nick);
}

void irc_disconnect() {
    sock_disconnect(&sock);
}

static void irc_set() {
    irc_disconnect();
    if (cmd_argc() < 5) {
        ui_output(cmd_client(), "Usage: %s nick host port channel\n", cmd_argv(0));
        return;
    }
    nick = cmd_argv(1);
    host = cmd_argv(2);
    port = atoi(cmd_argv(3));
    channel = cmd_argv(4);
    irc_connect();
}

void irc_init() {
    sock_init(&sock);
    cmd_add_global("irc", irc_set);
}

void irc_recv() {
    msg_t *msg = sock_recv_raw(&sock);
    if (!msg)
        return;

    char *buf = read_string(msg);
    char *end;
    while (*buf && (end = strchr(buf, '\r'))) {
        *end = '\0';
        if (!strncmp(buf, "PING", 4)) {
            irc_send("PONG %s", strchr(buf, ':'));
        } else if (buf[0] == ':') {
            char *user = NULL;
            char *command = NULL;
            char *where = NULL;
            char *message = NULL;
            char *p;
            int start = 1;
            int wordcount = 0;
            for (p = buf + start; *p; p++) {
                if (*p == ' ') {
                    if (*(p - 1) && *(p - 1) != ' ') {
                        if (wordcount < 4) {
                            wordcount++;
                            if (wordcount < 4)
                                *p = '\0';
                        }
                        switch (wordcount) {
                            case 1:
                                user = buf + start;
                                break;
                            case 2:
                                command = buf + start;
                                break;
                            case 3:
                                where = buf + start;
                                break;
                            case 4:
                                if (!message)
                                    message = buf + start;
                                break;
                        }
                        start += strlen(buf + start);
                    } else if (wordcount <= 4) {
                        switch (wordcount) {
                            case 1:
                                user++;
                                break;
                            case 2:
                                command++;
                                break;
                            case 3:
                                where++;
                                break;
                            case 4:
                                message++;
                                break;
                        }
                    }
                    start++;
                }
            }
            if (wordcount < 4) {
                wordcount++;
                if (wordcount < 4)
                    *p = '\0';
            }
            switch (wordcount) {
                case 1:
                    user = buf + start;
                    break;
                case 2:
                    command = buf + start;
                    break;
                case 3:
                    where = buf + start;
                    break;
                case 4:
                    if (!message)
                        message = buf + start;
                    break;
            }
            if (message)
                message++;

            if (!strncmp(command, "001", 3)) {
                irc_send("JOIN %s", channel);
            } else if (!strncmp(command, "JOIN", 4)) {
            } else if (!strncmp(command, "PRIVMSG", 7)) {
                if (where == NULL || message == NULL)
                    return;
                char *sep;
                if ((sep = strchr(user, '!')) != NULL)
                    *sep = '\0';
                char *target;
                if (where[0] == '#' || where[0] == '&' || where[0] == '+' || where[0] == '!')
                    target = where;
                else
                    target = user;
                static char event[MAX_ARGS_SIZE];
                sprintf(event, "ircrecv %s %s", target, message);
                cmd_execute_event(-2, event);
            }
        }
        buf = end + 2;
    }
}
