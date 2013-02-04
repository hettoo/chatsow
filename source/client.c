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
#include <unistd.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>

#include "config.h"
#include "utils.h"
#include "main.h"
#include "net.h"
#include "parser.h"
#include "cmd.h"
#include "client.h"

#define TIMEOUT 2000

#define NOP_TIME 5000

typedef enum client_state_e {
    CA_DISCONNECTED,
    CA_SETUP,
    CA_CHALLENGING,
    CA_CONNECTING,
    CA_CONNECTED,
    CA_LOADING,
    CA_CONFIGURING,
    CA_ENTERING,
    CA_ACTIVE
} client_state_t;

typedef struct client_s {
    int id;

    client_state_t state;

    parser_t parser;
    cs_t cs;

    sock_t sock;

    char host[512];
    char port[512];
    char name[512];
    int port_int;

    int command_seq;

    unsigned int resend;
    unsigned int last_send;

    int bitflags;
    int protocol;
    int spawn_count;
    int playernum;
    char motd[MAX_STRING_CHARS];
    char game[MAX_STRING_CHARS];
    char level[MAX_STRING_CHARS];
} client_t;

static client_t clients[CLIENT_SCREENS];

static void reset(client_t *c) {
    c->id = c - clients;

    c->parser.client = c->id;

    c->command_seq = 1;

    c->resend = 0;
    c->last_send = 0;

    c->protocol = 0;
    c->bitflags = 0;
    c->spawn_count = 0;
    c->playernum = 0;
    c->motd[0] = '\0';
    c->game[0] = '\0';
    c->level[0] = '\0';

    parser_reset(&c->parser);
    cs_init(&c->cs);
    sock_init(&c->sock);
}

void register_configstring_commands(client_t *c) {
    int max = CS_GAMECOMMANDS + MAX_GAMECOMMANDS;
    int i;
    for (i = CS_GAMECOMMANDS; i < max; i++)
        cmd_add_server(c->id, cs_get(&c->cs, i));
}

cs_t *client_cs(int id) {
    return &clients[id].cs;
}

qboolean client_active(int id) {
    return clients[id].state > CA_DISCONNECTED;
}

qboolean client_ready(int id) {
    return clients[id].state == CA_ACTIVE;
}

int get_bitflags(int id) {
    return clients[id].bitflags;
}

int get_playernum(int id) {
    return clients[id].playernum;
}

void set_spawn_count(int id, int new_spawn_count) {
    clients[id].spawn_count = new_spawn_count;
}

void set_protocol(int id, int new_protocol) {
    clients[id].protocol = new_protocol;
}

void set_game(int id, char *new_game) {
    strcpy(clients[id].game, new_game);
}

void set_playernum(int id, int new_playernum) {
    clients[id].playernum = new_playernum;
}

void set_level(int id, char *new_level) {
    strcpy(clients[id].level, new_level);
}

void set_bitflags(int id, int new_bitflags) {
    clients[id].bitflags = new_bitflags;
}

char *get_host(int id) {
    return clients[id].host;
}

static void client_title(client_t *c) {
    set_title(c->id, c->state == CA_DISCONNECTED ? "Disconnected" : c->motd, c->level, c->game, c->host, c->port);
}

static void set_state(client_t *c, int new_state) {
    c->state = new_state;
    c->resend = 0;
}

static void force_disconnect(client_t *c) {
    if (c->state == CA_DISCONNECTED)
        return;

    sock_disconnect(&c->sock);
    set_state(c, CA_DISCONNECTED);
    reset(c);
    client_title(c);
}

static void socket_connect(client_t *c) {
    if (c->state != CA_DISCONNECTED)
        force_disconnect(c);

    sock_connect(&c->sock, c->host, c->port_int);

    ui_output(c->id, "Connecting to %s:%s...\n", c->host, c->port);
    set_state(c, CA_SETUP);
}

static void challenge(client_t *c);

static void client_connect(client_t *c) {
    socket_connect(c);
    if (c->state == CA_DISCONNECTED)
        return;
    challenge(c);
}

void disconnect(int id) {
    client_t *c = clients + id;
    if (c->state == CA_DISCONNECTED)
        return;

    ui_output(id, "Disconnecting...\n");
    if (c->state >= CA_CONNECTING) {
        int i;
        for (i = 0; i < CERTAINTY; i++)
            client_command(id, "disconnect");
    }
    force_disconnect(c);
}

static void force_reconnect(client_t *c) {
    force_disconnect(c);
    client_connect(c);
}

static void reconnect(client_t *c) {
    disconnect(c->id);
    client_connect(c);
}

static void client_send(client_t *c) {
    sock_send(&c->sock);
    c->last_send = millis();
}

void client_say(int id, char *format, ...) {
    static char string[MAX_MSGLEN];
    strcpy(string, "say \"");
	va_list	argptr;
	va_start(argptr, format);
    vsprintf(string + strlen(string), format, argptr);
	va_end(argptr);
    strcat(string, "\"");
    cmd_execute(id, string);
}

void client_command(int id, char *format, ...) {
    client_t *c = clients + id;
    if (c->state < CA_SETUP) {
        ui_output(id, "not connected\n");
        return;
    }

    msg_t *msg = sock_init_send(&c->sock, qtrue);
    write_byte(msg, clc_clientcommand);
    if (!(c->bitflags & SV_BITFLAGS_RELIABLE))
        write_long(msg, c->command_seq++);
	va_list	argptr;
	va_start(argptr, format);
    vwrite_string(msg, format, argptr);
	va_end(argptr);
    client_send(c);
}

void client_ack(int id, int num) {
    client_t *c = clients + id;
    msg_t *msg = sock_init_send(&c->sock, qtrue);
    write_byte(msg, clc_svcack);
    write_long(msg, num);
    client_send(c);
}

static void client_recv(client_t *c) {
    msg_t *msg = sock_recv(&c->sock);
    if (msg) {
        if (c->sock.sequenced)
            parse_message(&c->parser, msg);
        else
            cmd_execute_from_server(c->id, read_string(msg));
    }
}

static void connection_request(client_t *c) {
    ui_output(c->id, "Sending connection request...\n");
    msg_t *msg = sock_init_send(&c->sock, qfalse);
    write_string(msg, "connect %d %s %s \"\\name\\%s\" 0", PROTOCOL, c->port, cmd_argv(1), c->name);
    client_send(c);

    set_state(c, CA_CONNECTING);
    c->resend = millis() + TIMEOUT;
}

static void challenge(client_t *c) {
    ui_output(c->id, "Requesting challenge...\n");
    msg_t *msg = sock_init_send(&c->sock, qfalse);
    write_string(msg, "getchallenge");
    client_send(c);

    set_state(c, CA_CHALLENGING);
    c->resend = millis() + TIMEOUT;
}

static void enter(client_t *c) {
    ui_output(c->id, "Entering the game...\n");
    client_command(c->id, "begin %d", c->spawn_count);

    set_state(c, CA_ENTERING);
    c->resend = millis() + TIMEOUT;
}

static void request_serverdata(client_t *c) {
    ui_output(c->id, "Sending serverdata request...\n");
    client_command(c->id, "new");

    set_state(c, CA_LOADING);
    c->resend = millis() + TIMEOUT;
}

void client_frame(int id) {
    client_t *c = clients + id;

    if (c->state == CA_DISCONNECTED)
        return;

    client_recv(c);
    switch (c->state) {
        case CA_CHALLENGING:
            if (millis() >= c->resend)
                challenge(c);
        break;
        case CA_CONNECTING:
            if (millis() >= c->resend)
                connection_request(c);
        break;
        case CA_LOADING:
            if (c->playernum == 0) {
                if (millis() >= c->resend)
                    request_serverdata(c);
                break;
            } else {
                cs_init(&c->cs);
                set_state(c, CA_CONFIGURING);
            }
        case CA_CONFIGURING:
            if (millis() >= c->resend) {
                ui_output(c->id, "Requesting configstrings...\n");
                client_command(c->id, "configstrings %d 0", c->spawn_count);
                c->resend = millis() + TIMEOUT;
            }
            break;
        case CA_ENTERING:
            if (millis() >= c->resend)
                enter(c);
            break;
        case CA_ACTIVE:
            if (millis() >= c->last_send + NOP_TIME) {
                msg_t *msg = sock_init_send(&c->sock, qtrue);
                write_byte(msg, clc_nop);
                client_send(c);
            }
            break;
        default:
            break;
    }
}

static void set_server(client_t *c, char *new_host, char *new_port) {
    disconnect(c->id);
    if (new_host == NULL || new_port == NULL) {
        strcpy(c->host, "");
        strcpy(c->port, "");
        return;
    }
    strcpy(c->host, new_host);
    if (!strcmp(c->host, "localhost"))
        strcpy(c->host, "127.0.0.1");
    strcpy(c->port, new_port);
    c->port_int = atoi(c->port);
    client_connect(c);
    client_title(c);
}

void cmd_challenge() {
    client_t *c = clients + cmd_client();
    if (c->state != CA_CHALLENGING)
        return;

    connection_request(c);
}

void cmd_client_connect() {
    client_t *c = clients + cmd_client();
    request_serverdata(c);
}

void cmd_cs() {
    client_t *c = clients + cmd_client();
    int i;
    for (i = 0; i < cmd_argc(); i++) {
        if (i % 2 == 1)
            cs_set(&c->cs, atoi(cmd_argv(i)), cmd_argv(i + 1));
    }
    strcpy(c->name, player_name(&c->cs, c->playernum));
    set_status(c->id, c->name, cs_get(&c->cs, 0));
}

void cmd_cmd() {
    client_t *c = clients + cmd_client();
    client_command(c->id, "%s", cmd_args(1));
    c->resend = millis() + TIMEOUT;
}

void cmd_precache() {
    client_t *c = clients + cmd_client();
    if (c->state != CA_CONFIGURING || cs_get(&c->cs, 0)[0] == '\0')
        return;

    enter(c);
}

void client_activate(int id) {
    client_t *c = clients + id;
    if (c->state != CA_ENTERING)
        return;

    set_state(c, CA_ACTIVE);
    client_title(c);
    cmd_execute(id, "players");
}

static void cmd_disconnect() {
    client_t *c = clients + cmd_client();
    disconnect(c->id);
}

static void cmd_reconnect() {
    client_t *c = clients + cmd_client();
    reconnect(c);
}

static void cmd_nop() {
}

static void chat_command(client_t *c, char *command) {
    if (command[0] == '!')
        cmd_execute_public(c->id, command + 1);
}

static void cmd_pr() {
    client_t *c = clients + cmd_client();
    ui_output(c->id, "%s", cmd_argv(1));
}

static void cmd_ch() {
    client_t *c = clients + cmd_client();
    char *name = player_name(&c->cs, atoi(cmd_argv(1)));
    ui_output_important(c->id, "%s^7: ^2%s^7\n", name, cmd_argv(2));
    chat_command(c, cmd_argv(2));
}

static void cmd_tch() {
    client_t *c = clients + cmd_client();
    char *name = player_name(&c->cs, atoi(cmd_argv(1)));
    ui_output_important(c->id, "%s^7: ^3%s^7\n", name, cmd_argv(2));
    chat_command(c, cmd_argv(2));
}

static void cmd_tvch() {
    client_t *c = clients + cmd_client();
    char *name = player_name(&c->cs, atoi(cmd_argv(1)));
    ui_output_important(c->id, "[TV]%s^7: ^2%s^7\n", name, cmd_argv(2));
    chat_command(c, cmd_argv(2));
}

static void cmd_motd() {
    client_t *c = clients + cmd_client();
    strcpy(c->motd, cmd_argv(2));
}

static void cmd_connect() {
    client_t *c = clients + cmd_client();
    char *new_host = cmd_argv(1);
    char *new_port = "44400";
    if (cmd_argc() > 2)
        new_port = cmd_argv(2);
    set_server(c, new_host, new_port);
}

static void cmd_name() {
    client_t *c = clients + cmd_client();
    strcpy(c->name, cmd_argv(1));
    set_status(c->id, c->name, cs_get(&c->cs, 0));
    if (c->state >= CA_SETUP)
        client_command(c->id, "usri \"\\name\\%s\"", c->name);
}

static char suggestions[MAX_CMDS][MAX_SUGGESTION_SIZE];

static qboolean suggestion_remove_test(void *x) {
    char *suggestion = (char *)x;
    int i;
    for (i = 0; suggestions[i] != suggestion; i++) {
        if (!strcmp(suggestion, suggestions[i]))
            return qtrue;
    }
    return qfalse;
}

static void cmd_help_public() {
    static char message[MAX_CMDS * MAX_SUGGESTION_SIZE];
    int suggestion_count = cmd_suggest(cmd_client(), "", suggestions, qtrue);
    qsort(suggestions, suggestion_count, MAX_SUGGESTION_SIZE, insensitive_cmp);
    rm(suggestions, sizeof(suggestions[0]), &suggestion_count, suggestion_remove_test);
    message[0] = '\0';
    int line = 0;
    int i;
    for (i = 0; i < suggestion_count; i++) {
        int len = strlen(suggestions[i]);
        if (line + len > 80) {
            strcat(message, "\n");
            line = 0;
        }
        strcat(message, suggestions[i]);
        strcat(message, " ");
        line += len + 1;
    }
    client_say(cmd_client(), "%s", message);
}

void client_register_commands() {
    cmd_add_from_server("challenge", cmd_challenge);
    cmd_add_from_server("client_connect", cmd_client_connect);
    cmd_add_from_server("cs", cmd_cs);
    cmd_add_from_server("cmd", cmd_cmd);
    cmd_add_from_server("precache", cmd_precache);
    cmd_add_from_server("disconnect", cmd_disconnect);
    cmd_add_from_server("forcereconnect", cmd_reconnect);
    cmd_add_from_server("reconnect", cmd_reconnect);

    cmd_add_from_server("mm", cmd_nop);
    cmd_add_from_server("plstats", cmd_nop);
    cmd_add_from_server("scb", cmd_nop);
    cmd_add_from_server("cvarinfo", cmd_nop);
    cmd_add_from_server("obry", cmd_nop);
    cmd_add_from_server("ti", cmd_nop);
    cmd_add_from_server("changing", cmd_nop);
    cmd_add_from_server("cp", cmd_nop);

    cmd_add_from_server("dstart", cmd_nop);
    cmd_add_from_server("dstop", cmd_nop);
    cmd_add_from_server("dcancel", cmd_nop);

    cmd_add_from_server("pr", cmd_pr);
    cmd_add_from_server("ch", cmd_ch);
    cmd_add_from_server("tch", cmd_tch);
    cmd_add_from_server("tvch", cmd_tvch);
    cmd_add_from_server("motd", cmd_motd);

    cmd_add_find_free("connect", cmd_connect);
    cmd_add_broadcast("name", cmd_name);
    cmd_add_public_generic("help", cmd_help_public);
}

int player_suggest(int id, char *partial, char suggestions[][MAX_SUGGESTION_SIZE]) {
    int count = 0;
    int i;
    for (i = 1; i <= MAX_CLIENTS; i++) {
        char *name = player_name(&clients[id].cs, i);
        if (name && *name && partial_match(partial, name))
            strcpy(suggestions[count++], name);
    }
    return count;
}

void client_start(int id) {
    client_t *c = clients + id;
    reset(c);
    register_configstring_commands(c);
    cmd_add_persistent(id, "reconnect", cmd_reconnect);
    cmd_add(id, "disconnect", cmd_disconnect);

    strcpy(c->name, "chatter");
    set_state(c, CA_DISCONNECTED);
    set_server(c, NULL, NULL);
    set_status(c->id, c->name, cs_get(&c->cs, 0));
}

void demoinfo_key(int id, char *key) {
    ui_output(id, "demoinfo key %s\n", key);
}

void demoinfo_value(int id, char *value) {
    ui_output(id, "demoinfo value %s\n", value);
}

void execute(int id, char *cmd, qbyte *targets, int target_count) {
    client_t *c = clients + id;
    if (target_count > 0) {
        int i;
        qboolean found = qfalse;
        for (i = 0; i < target_count; i++) {
            if (targets[i] == c->playernum)
                found = qtrue;
        }
        if (!found)
            return;
    }
    cmd_execute_from_server(id, cmd);
}
