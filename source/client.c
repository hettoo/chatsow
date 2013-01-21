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

#include "import.h"
#include "config.h"
#include "utils.h"
#include "ui.h"
#include "parser.h"
#include "cmd.h"
#include "cs.h"
#include "client.h"

#define TIMEOUT 1400

#define MAX_DROPS 10

#define NOP_TIME 5000

typedef enum client_state_s {
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

static client_state_t state;

static char *host;
static char *port;
static char *name;
static int port_int;
static struct sockaddr_in serv_addr;
static int sockfd;
static socklen_t slen = sizeof(serv_addr);

static int drops;

static int outseq;
static int inseq;

static int command_seq;

static unsigned int last_status;

static unsigned int resend;
static unsigned int last_send;

static int fragment_total;
static qbyte fragment_buffer[MAX_MSGLEN];

static int bitflags;
static int protocol;
static int spawn_count;
static int playernum;
static char game[MAX_STRING_CHARS];
static char level[MAX_STRING_CHARS];

static msg_t smsg;

static void reset() {
    drops = 0;

    outseq = 1;
    inseq = 0;

    command_seq = 1;

    last_status = 0;

    resend = 0;
    last_send = 0;

    fragment_total = 0;

    protocol = 0;
    bitflags = 0;
    spawn_count = 0;
    playernum = 0;
    game[0] = '\0';
    level[0] = '\0';

    parser_reset();
}

qboolean client_ready() {
    return state == CA_ACTIVE;
}

int get_bitflags() {
    return bitflags;
}

void set_spawn_count(int new_spawn_count) {
    spawn_count = new_spawn_count;
}

void set_protocol(int new_protocol) {
    protocol = new_protocol;
}

void set_game(char *new_game) {
    strcpy(game, new_game);
}

void set_playernum(int new_playernum) {
    playernum = new_playernum;
}

void set_level(char *new_level) {
    strcpy(level, new_level);
}

void set_bitflags(int new_bitflags) {
    bitflags = new_bitflags;
}

static void client_title() {
    set_title(cs_get(0), level, game, host, port);
}

static void msg_clear(msg_t *msg) {
    msg->readcount = 0;
    msg->cursize = 0;
    msg->maxsize = sizeof(msg->data);
    msg->compressed = qfalse;
}

static void set_state(int new_state) {
    state = new_state;
    resend = 0;
}

static void force_disconnect() {
    close(sockfd);
    set_state(CA_DISCONNECTED);
    client_title();
}

static void socket_connect() {
    if (state != CA_DISCONNECTED)
        force_disconnect();

    if ((sockfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1)
        die("socket");

    bzero(&serv_addr, sizeof(serv_addr));

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(port_int);

    if (inet_aton(host, &serv_addr.sin_addr) == 0)
        die("inet_aton");

    ui_output("Connecting to %s:%s...\n", host, port);
    set_state(CA_SETUP);
}

static void msg_init(msg_t *msg) {
    msg_clear(msg);
    write_long(msg, outseq++);
    write_long(msg, inseq);
    write_short(msg, port_int);
}

static void msg_init_outofband(msg_t *msg) {
    msg_clear(msg);
    write_long(msg, -1);
}

static void challenge();

static void client_connect() {
    socket_connect();
    reset();
    challenge();
}

void disconnect() {
    ui_output("Disconnecting...\n");
    if (state >= CA_CONNECTING) {
        int i;
        for (i = 0; i < CERTAINTY; i++)
            client_command("disconnect");
    }
    force_disconnect();
}

static void force_reconnect() {
    force_disconnect();
    client_connect();
}

static void reconnect() {
    disconnect();
    client_connect();
}

void drop(char *msg) {
    drops++;
    if (drops == MAX_DROPS) {
        ui_output("^5too many drops, disconnecting...\n");
        force_disconnect();
    } else {
        ui_output("^5%s, reconnecting %d...\n", msg, drops);
        force_reconnect();
    }
}

static void client_send(msg_t *msg) {
    if (sendto(sockfd, msg->data, msg->cursize, 0, (struct sockaddr*)&serv_addr, slen) == -1)
        drop("sendto failed");
    last_send = millis();
}

void client_command(char *format, ...) {
    static char string[MAX_MSGLEN];
	va_list	argptr;
	va_start(argptr, format);
    vsprintf(string, format, argptr);
	va_end(argptr);
    msg_t msg;
    msg_init(&msg);
    write_byte(&msg, clc_clientcommand);
    if (!(bitflags & SV_BITFLAGS_RELIABLE))
        write_long(&msg, command_seq++);
    write_string(&msg, string);
    client_send(&msg);
}

void client_ack(int num) {
    msg_t msg;
    msg_init(&msg);
    write_byte(&msg, clc_svcack);
    write_long(&msg, num);
    client_send(&msg);
}

static void client_recv() {
    static msg_t msg;
    msg_clear(&msg);
    if ((msg.cursize = recvfrom(sockfd, msg.data, msg.maxsize, MSG_DONTWAIT, (struct sockaddr*)&serv_addr, &slen)) == -1) {
        if (errno == EAGAIN)
            return;
        drop("recvfrom failed");
    }
    int seq = read_long(&msg);
    if (seq == -1) {
        cmd_execute(read_string(&msg));
        return;
    }
    qboolean compressed = qfalse;
    qboolean fragmented = qfalse;
    if (seq & FRAGMENT_BIT) {
        seq &= ~FRAGMENT_BIT;
        fragmented = qtrue;
    }
    inseq = seq;
    int ack = read_long(&msg);
    if (ack & FRAGMENT_BIT) {
        ack &= ~FRAGMENT_BIT;
        compressed = qtrue;
    }
    if (fragmented) {
        short fragment_start = read_short(&msg);
        short fragment_length = read_short(&msg);
        if (fragment_start != fragment_total) {
            msg.readcount = 0;
            msg.cursize = 0;
            return;
        }
        qboolean last = qfalse;
        if (fragment_length & FRAGMENT_LAST) {
            fragment_length &= ~FRAGMENT_LAST;
            last = qtrue;
        }
        memcpy(fragment_buffer + fragment_total, msg.data + msg.readcount, fragment_length);
        fragment_total += fragment_length;
        msg.readcount = 0;
        if (last) {
            memcpy(msg.data, fragment_buffer, fragment_total);
            msg.cursize = fragment_total;
            fragment_total = 0;
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
    parse_message(&msg);
}

static void connection_request() {
    ui_output("Sending connection request...\n");
    msg_init_outofband(&smsg);
    write_string(&smsg, "connect 15 ");
    smsg.cursize--;
    write_string(&smsg, port);
    smsg.cursize--;
    write_string(&smsg, " ");
    smsg.cursize--;
    write_string(&smsg, cmd_argv(1));
    smsg.cursize--;
    write_string(&smsg, " \"\\name\\");
    smsg.cursize--;
    write_string(&smsg, name);
    smsg.cursize--;
    write_string(&smsg, "\" 0");
    client_send(&smsg);

    set_state(CA_CONNECTING);
    resend = millis() + TIMEOUT;
}

static void challenge() {
    ui_output("Requesting challenge...\n");
    msg_t msg;
    msg_init_outofband(&msg);
    write_string(&msg, "getchallenge");
    client_send(&msg);

    set_state(CA_CHALLENGING);
    resend = millis() + TIMEOUT;
}

static void enter() {
    ui_output("Entering the game...\n");
    client_command("begin %d", spawn_count);

    set_state(CA_ENTERING);
    resend = millis() + TIMEOUT;
}

static void request_serverdata() {
    ui_output("Sending serverdata request...\n");
    client_command("new");

    set_state(CA_LOADING);
    resend = millis() + TIMEOUT;
}

void client_frame() {
    if (state == CA_DISCONNECTED)
        return;

    client_recv();
    int m = millis();
    if (last_status == 0 || m >= last_status + 1000) {
        draw_status(name);
        last_status = m;
    }
    switch (state) {
        case CA_CHALLENGING:
            if (millis() >= resend)
                challenge();
        break;
        case CA_CONNECTING:
            if (millis() >= resend)
                connection_request();
        break;
        case CA_LOADING:
            if (playernum == 0) {
                if (millis() >= resend)
                    request_serverdata();
                break;
            } else {
                cs_init();
                set_state(CA_CONFIGURING);
            }
        case CA_CONFIGURING:
            if (millis() >= resend) {
                ui_output("Requesting configstrings...\n");
                client_command("configstrings %d 0", spawn_count);
                resend = millis() + TIMEOUT;
            }
            break;
        case CA_ENTERING:
            if (millis() >= resend)
                enter();
            break;
        case CA_ACTIVE:
            if (millis() >= last_send + NOP_TIME) {
                msg_t msg;
                msg_init(&msg);
                write_byte(&msg, clc_nop);
                client_send(&msg);
            }
            break;
        default:
            break;
    }
}

void cmd_challenge() {
    if (state != CA_CHALLENGING)
        return;

    connection_request();
}

void cmd_client_connect() {
    request_serverdata();
}

void cmd_cs() {
    int i;
    for (i = 0; i < cmd_argc(); i++) {
        if (i % 2 == 1)
            cs_set(atoi(cmd_argv(i)), cmd_argv(i + 1));
    }
}

void cmd_cmd() {
    client_command("%s", cmd_args(1));
    resend = millis() + TIMEOUT;
}

void cmd_precache() {
    if (state != CA_CONFIGURING || cs_get(0)[0] == '\0')
        return;

    enter();
}

void client_activate() {
    if (state != CA_ENTERING)
        return;

    set_state(CA_ACTIVE);
    drops = 0;
    client_title();
    cmd_execute("players");
}

void cmd_disconnect() {
    disconnect();
}

void cmd_reconnect() {
    reconnect();
}

void cmd_nop() {
}

void cmd_pr() {
    ui_output("%s^7", cmd_argv(1));
}

void cmd_ch() {
    ui_output("%s^7: ^2%s^7\n", player_name(atoi(cmd_argv(1))), cmd_argv(2));
}

void cmd_tvch() {
    ui_output("[TV]%s^7: ^2%s^7\n", cmd_argv(1), cmd_argv(2));
}

void cmd_motd() {
    ui_output("^5Message of the day:^7\n%s", cmd_argv(2));
}

void cmd_players() {
    int i;
    qboolean first = qtrue;
    for (i = 1; i <= MAX_CLIENTS; i++) {
        char *name = player_name(i);
        if (name && *name) {
            if (first) {
                ui_output("^5Online players:^7");
                first = qfalse;
            }
            ui_output(" %s^7", name);
        }
    }
    ui_output("\n");
}

void client_start(char *new_host, char *new_port, char *new_name) {
    cmd_add("challenge", cmd_challenge);
    cmd_add("client_connect", cmd_client_connect);
    cmd_add("cs", cmd_cs);
    cmd_add("cmd", cmd_cmd);
    cmd_add("precache", cmd_precache);
    cmd_add("disconnect", cmd_disconnect);
    cmd_add("forcereconnect", cmd_reconnect);
    cmd_add("reconnect", cmd_reconnect);

    cmd_add("mm", cmd_nop);
    cmd_add("plstats", cmd_nop);
    cmd_add("scb", cmd_nop);
    cmd_add("cvarinfo", cmd_nop);
    cmd_add("obry", cmd_nop);
    cmd_add("ti", cmd_nop);
    cmd_add("changing", cmd_nop);

    cmd_add("dstart", cmd_nop);
    cmd_add("dstop", cmd_nop);
    cmd_add("dcancel", cmd_nop);

    cmd_add("pr", cmd_pr);
    cmd_add("cp", cmd_pr);
    cmd_add("ch", cmd_ch);
    cmd_add("tch", cmd_ch);
    cmd_add("tvch", cmd_tvch);
    cmd_add("motd", cmd_motd);

    cmd_add("players", cmd_players);

    host = new_host;
    port = new_port;
    name = new_name;
    port_int = atoi(port);
    set_state(CA_DISCONNECTED);
    client_title();
    client_connect();
}

void demoinfo_key(char *key) {
    ui_output("demoinfo key %s\n", key);
}

void demoinfo_value(char *value) {
    ui_output("demoinfo value %s\n", value);
}

void execute(char *cmd, qbyte *targets, int target_count) {
    if (target_count > 0) {
        int i;
        qboolean found = qfalse;
        for (i = 0; i < target_count; i++) {
            if (targets[i] == playernum)
                found = qtrue;
        }
        if (!found)
            return;
    }
    cmd_execute(cmd);
}
