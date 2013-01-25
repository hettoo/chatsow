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

#include <libnotify/notify.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <errno.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include "zlib.h"

#include "config.h"
#include "utils.h"
#include "main.h"
#include "net.h"
#include "parser.h"
#include "cmd.h"
#include "client.h"

#define TIMEOUT 2000

#define MAX_DROPS 10

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

    char host[512];
    char port[512];
    char name[512];
    int port_int;
    struct sockaddr_in serv_addr;
    int sockfd;
    socklen_t slen;

    int drops;

    int outseq;
    int inseq;

    int command_seq;

    unsigned int last_status;

    unsigned int resend;
    unsigned int last_send;

    int fragment_total;
    qbyte fragment_buffer[MAX_MSGLEN];

    int bitflags;
    int protocol;
    int spawn_count;
    int playernum;
    char motd[MAX_STRING_CHARS];
    char game[MAX_STRING_CHARS];
    char level[MAX_STRING_CHARS];
} client_t;

static client_t clients[CLIENT_SCREENS];

static msg_t smsg;

static void reset(client_t *c) {
    c->id = c - clients;

    c->parser.client = c->id;

    c->slen = sizeof(c->serv_addr);

    c->drops = 0;

    c->outseq = 1;
    c->inseq = 0;

    c->command_seq = 1;

    c->last_status = 0;

    c->resend = 0;
    c->last_send = 0;

    c->fragment_total = 0;

    c->protocol = 0;
    c->bitflags = 0;
    c->spawn_count = 0;
    c->playernum = 0;
    c->motd[0] = '\0';
    c->game[0] = '\0';
    c->level[0] = '\0';

    parser_reset(&c->parser);
    cs_init(&c->cs);
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

static void client_title(client_t *c) {
    set_title(c->id, c->motd, c->level, c->game, c->host, c->port);
}

static void set_state(client_t *c, int new_state) {
    c->state = new_state;
    c->resend = 0;
}

static void force_disconnect(client_t *c) {
    if (c->state == CA_DISCONNECTED)
        return;

    close(c->sockfd);
    set_state(c, CA_DISCONNECTED);
    client_title(c);
}

static void socket_connect(client_t *c) {
    if (c->state != CA_DISCONNECTED)
        force_disconnect(c);

    if ((c->sockfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1) {
        ui_output(c->id, "Unable to create socket.\n");
        return;
    }

    bzero(&c->serv_addr, sizeof(c->serv_addr));

    c->serv_addr.sin_family = AF_INET;
    c->serv_addr.sin_port = htons(c->port_int);

    if (inet_aton(c->host, &c->serv_addr.sin_addr) == 0) {
        ui_output(c->id, "Invalid hostname.\n");
        return;
    }

    ui_output(c->id, "Connecting to %s:%s...\n", c->host, c->port);
    set_state(c, CA_SETUP);
}

static void msg_init(client_t *c, msg_t *msg) {
    msg_clear(msg);
    write_long(msg, c->outseq++);
    write_long(msg, c->inseq);
    write_short(msg, c->port_int);
}

static void challenge(client_t *c);

static void client_connect(client_t *c) {
    socket_connect(c);
    if (c->state == CA_DISCONNECTED)
        return;
    reset(c);
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

static void drop(client_t *c, char *msg) {
    c->drops++;
    if (c->drops == MAX_DROPS) {
        ui_output(c->id, "^5too many drops, disconnecting...\n");
        force_disconnect(c);
    } else {
        ui_output(c->id, "^5%s, reconnecting %d...\n", msg, c->drops);
        force_reconnect(c);
    }
}

static void client_send(client_t *c, msg_t *msg) {
    if (sendto(c->sockfd, msg->data, msg->cursize, 0, (struct sockaddr*)&c->serv_addr, c->slen) == -1)
        drop(c, "sendto failed");
    c->last_send = millis();
}

void client_command(int id, char *format, ...) {
    client_t *c = clients + id;
    if (c->state < CA_SETUP) {
        ui_output(id, "not connected\n");
        return;
    }

    static char string[MAX_MSGLEN];
	va_list	argptr;
	va_start(argptr, format);
    vsprintf(string, format, argptr);
	va_end(argptr);

    msg_init(c, &smsg);
    write_byte(&smsg, clc_clientcommand);
    if (!(c->bitflags & SV_BITFLAGS_RELIABLE))
        write_long(&smsg, c->command_seq++);
    write_string(&smsg, string);
    client_send(c, &smsg);
}

void client_ack(int id, int num) {
    client_t *c = clients + id;
    msg_init(c, &smsg);
    write_byte(&smsg, clc_svcack);
    write_long(&smsg, num);
    client_send(c, &smsg);
}

static void client_recv(client_t *c) {
    static msg_t msg;
    msg_clear(&msg);
    if ((msg.cursize = recvfrom(c->sockfd, msg.data, msg.maxsize, MSG_DONTWAIT, (struct sockaddr*)&c->serv_addr, &c->slen)) == -1) {
        if (errno == EAGAIN)
            return;
        drop(c, "recvfrom failed");
    }
    int seq = read_long(&msg);
    if (seq == -1) {
        cmd_execute_from_server(c->id, read_string(&msg));
        return;
    }
    qboolean compressed = qfalse;
    qboolean fragmented = qfalse;
    if (seq & FRAGMENT_BIT) {
        seq &= ~FRAGMENT_BIT;
        fragmented = qtrue;
    }
    c->inseq = seq;
    int ack = read_long(&msg);
    if (ack & FRAGMENT_BIT) {
        ack &= ~FRAGMENT_BIT;
        compressed = qtrue;
    }
    if (fragmented) {
        short fragment_start = read_short(&msg);
        short fragment_length = read_short(&msg);
        if (fragment_start != c->fragment_total) {
            msg.readcount = 0;
            msg.cursize = 0;
            return;
        }
        qboolean last = qfalse;
        if (fragment_length & FRAGMENT_LAST) {
            fragment_length &= ~FRAGMENT_LAST;
            last = qtrue;
        }
        read_data(&msg, c->fragment_buffer + c->fragment_total, fragment_length);
        c->fragment_total += fragment_length;
        msg.readcount = 0;
        if (last) {
            memcpy(msg.data, c->fragment_buffer, c->fragment_total);
            msg.cursize = c->fragment_total;
            c->fragment_total = 0;
        } else {
            msg.cursize = 0;
            return;
        }
    }
    if (compressed && msg.cursize - msg.readcount > 0) {
        static qbyte temp[MAX_MSGLEN];
        unsigned long new_size = MAX_MSGLEN;
        uncompress(temp, &new_size, msg.data + msg.readcount, msg.cursize - msg.readcount);
        memcpy(msg.data, temp, new_size);
        msg.readcount = 0;
        msg.cursize = new_size;
    }
    parse_message(&c->parser, &msg);
}

static void connection_request(client_t *c) {
    ui_output(c->id, "Sending connection request...\n");
    msg_init_outofband(&smsg);
    write_string(&smsg, "connect 15 ");
    smsg.cursize--;
    write_string(&smsg, c->port);
    smsg.cursize--;
    write_string(&smsg, " ");
    smsg.cursize--;
    write_string(&smsg, cmd_argv(1));
    smsg.cursize--;
    write_string(&smsg, " \"\\name\\");
    smsg.cursize--;
    write_string(&smsg, c->name);
    smsg.cursize--;
    write_string(&smsg, "\" 0");
    client_send(c, &smsg);

    set_state(c, CA_CONNECTING);
    c->resend = millis() + TIMEOUT;
}

static void challenge(client_t *c) {
    ui_output(c->id, "Requesting challenge...\n");
    msg_init_outofband(&smsg);
    write_string(&smsg, "getchallenge");
    client_send(c, &smsg);

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
    int m = millis();
    if (c->last_status == 0 || m >= c->last_status + 1000) {
        draw_status(id, c->name, cs_get(&c->cs, 0));
        c->last_status = m;
    }

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
                msg_init(c, &smsg);
                write_byte(&smsg, clc_nop);
                client_send(c, &smsg);
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
    c->drops = 0;
    client_title(c);
    cmd_execute(id, "players");
}

void cmd_disconnect() {
    client_t *c = clients + cmd_client();
    disconnect(c->id);
}

void cmd_reconnect() {
    client_t *c = clients + cmd_client();
    reconnect(c);
}

void cmd_nop() {
}

void cmd_pr() {
    client_t *c = clients + cmd_client();
    ui_output(c->id, "%s^7", cmd_argv(1));
}

static void notify_chat(int id, char *name, char *text) {
    notify_init("wrlc");
    static char title[] = "wrlc(x)";
    title[strlen(title) - 2] = '1' + id;
    static char message[MAX_MSGLEN + MAX_CONFIGSTRING_CHARS + 2];
    sprintf(message, "%s: %s", uncolor(name), text);
    NotifyNotification* notification = notify_notification_new(title, message, NULL);
    notify_notification_set_timeout(notification, 2000);
    notify_notification_show(notification, NULL);
    g_object_unref(G_OBJECT(notification));
}

void cmd_ch() {
    client_t *c = clients + cmd_client();
    char *name = player_name(&c->cs, atoi(cmd_argv(1)));
    ui_output(c->id, "%s^7: ^2%s^7\n", name, cmd_argv(2));
    ui_set_important(c->id);
    notify_chat(c->id, name, cmd_argv(2));
}

void cmd_tch() {
    client_t *c = clients + cmd_client();
    char *name = player_name(&c->cs, atoi(cmd_argv(1)));
    ui_output(c->id, "%s^7: ^3%s^7\n", name, cmd_argv(2));
    ui_set_important(c->id);
    notify_chat(c->id, name, cmd_argv(2));
}

void cmd_tvch() {
    client_t *c = clients + cmd_client();
    char *name = player_name(&c->cs, atoi(cmd_argv(1)));
    ui_output(c->id, "[TV]%s^7: ^2%s^7\n", name, cmd_argv(2));
    ui_set_important(c->id);
    notify_chat(c->id, name, cmd_argv(2));
}

void cmd_motd() {
    client_t *c = clients + cmd_client();
    strcpy(c->motd, cmd_argv(2));
}

void cmd_connect() {
    client_t *c = clients + cmd_client();
    char *new_host = cmd_argv(1);
    char *new_port = "44400";
    if (cmd_argc() > 2)
        new_port = cmd_argv(2);
    set_server(c, new_host, new_port);
}

void cmd_name() {
    client_t *c = clients + cmd_client();
    strcpy(c->name, cmd_argv(1));
    if (c->state >= CA_SETUP)
        client_command(c->id, "usri \"\\name\\%s\"", c->name);
}

void cmd_quit() {
    quit();
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
    cmd_add_global("quit", cmd_quit);
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
    cmd_add(id, "reconnect", cmd_reconnect);
    cmd_add(id, "disconnect", cmd_disconnect);

    strcpy(c->name, "chatter");
    set_state(c, CA_DISCONNECTED);
    set_server(c, NULL, NULL);
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
