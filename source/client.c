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

#include <pthread.h>
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

static qboolean started = qfalse;
static pthread_t thread;
static pthread_mutex_t stop_mutex;
static qboolean stopping = qfalse;
static pthread_mutex_t command_mutex;

static char *host;
static char *port;
static int port_int;
static struct sockaddr_in serv_addr;
static int sockfd;
static socklen_t slen = sizeof(serv_addr);

static int outseq = 1;
static int inseq = 0;

static msg_t smsg;

typedef enum client_state_s {
    CA_DISCONNECTED,
    CA_SETUP,
    CA_CHALLENGING,
    CA_CONNECTING,
    CA_CONNECTED,
    CA_LOADING,
    CA_CONFIGURING,
    CA_ACTIVE
} client_state_t;

static client_state_t state;

static void msg_clear(msg_t *msg) {
    msg->readcount = 0;
    msg->cursize = 0;
    msg->maxsize = sizeof(msg->data);
    msg->compressed = qfalse;
}

static void client_connect() {
    if (state != CA_DISCONNECTED)
        die("not disconnected");

    if ((sockfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1)
        die("socket");

    bzero(&serv_addr, sizeof(serv_addr));

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(port_int);

    if (inet_aton(host, &serv_addr.sin_addr) == 0)
        die("inet_aton");

    state = CA_SETUP;
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

static void client_send(msg_t *msg) {
    if (sendto(sockfd, msg->data, msg->cursize, 0, (struct sockaddr*)&serv_addr, slen) == -1)
        die("sendto");
}

static void client_command(char *format, ...) {
    pthread_mutex_lock(&command_mutex);
    static int n = 1;
    static char string[MAX_MSGLEN];
	va_list	argptr;
	va_start(argptr, format);
    vsprintf(string, format, argptr);
	va_end(argptr);
    msg_t msg;
    msg_init(&msg);
    write_byte(&msg, clc_clientcommand);
    if (!connection_reliable())
        write_long(&msg, n++);
    write_string(&msg, string);
    client_send(&msg);
    pthread_mutex_unlock(&command_mutex);
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
        die("recvfrom");
    }
    int seq = read_long(&msg);
    //ui_output("%d %d\n", msg.cursize, seq);
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
        static int fragment_total = 0;
        static qbyte buffer[MAX_MSGLEN];
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
        memcpy(buffer + fragment_total, msg.data + msg.readcount, fragment_length);
        fragment_total += fragment_length;
        msg.readcount = 0;
        if (last) {
            memcpy(msg.data, buffer, fragment_total);
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

static void client_frame() {
    if (state == CA_LOADING && player_num() != 0) {
        ui_output("requesting configstrings\n");
        state = CA_CONFIGURING;
        client_command("configstrings %d 0", spawn_count());
    }
}

static void *client_run(void *args) {
    client_connect();

    ui_output("requesting challenge\n");
    msg_t msg;
    msg_init_outofband(&msg);
    write_string(&msg, "getchallenge");
    client_send(&msg);
    state = CA_CHALLENGING;

    while (1) {
        client_recv();
        client_frame();
        pthread_mutex_lock(&stop_mutex);
        if (stopping)
            break;
        pthread_mutex_unlock(&stop_mutex);
    }
    pthread_mutex_unlock(&stop_mutex);

    int i;
    for (i = 0; i < CERTAINTY; i++)
        client_command("disconnect");
    close(sockfd);

    return NULL;
}

void cmd_challenge() {
    if (state != CA_CHALLENGING)
        die("not challenging");

    ui_output("sending connection request\n");
    msg_init_outofband(&smsg);
    write_string(&smsg, "connect 15 ");
    smsg.cursize--;
    write_string(&smsg, port);
    smsg.cursize--;
    write_string(&smsg, " ");
    smsg.cursize--;
    write_string(&smsg, cmd_argv(1));
    smsg.cursize--;
    write_string(&smsg, " \"\\name\\chattoo\" 0");
    client_send(&smsg);

    state = CA_CONNECTING;
}

void cmd_client_connect() {
    if (state != CA_CONNECTING)
        die("not connecting");

    ui_output("sending serverdata request\n");
    client_command("new");
    state = CA_LOADING;
}

void cmd_cs() {
}

void cmd_cmd() {
    client_command("%s", cmd_args(1));
}

void cmd_precache() {
    ui_output("entering the game\n");
    client_command("begin %d", spawn_count());
}

void cmd_nop() {
}

void cmd_pr() {
    ui_output("%s", cmd_argv(1));
}

void client_start(char *new_host, char *new_port) {
    cmd_add("challenge", cmd_challenge);
    cmd_add("client_connect", cmd_client_connect);
    cmd_add("cs", cmd_cs);
    cmd_add("cmd", cmd_cmd);
    cmd_add("precache", cmd_precache);

    cmd_add("plstats", cmd_nop);
    cmd_add("scb", cmd_nop);
    cmd_add("cvarinfo", cmd_nop);

    cmd_add("pr", cmd_pr);

    host = new_host;
    port = new_port;
    port_int = atoi(port);
    state = CA_DISCONNECTED;
    pthread_mutex_init(&stop_mutex, NULL);
    pthread_mutex_init(&command_mutex, NULL);
    stopping = qfalse;
    pthread_create(&thread, NULL, client_run, NULL);
    started = qtrue;
}

void client_stop() {
    if (started) {
        pthread_mutex_lock(&stop_mutex);
        stopping = qtrue;
        pthread_mutex_unlock(&stop_mutex);
        pthread_join(thread, NULL);
        started = qfalse;
        pthread_mutex_destroy(&stop_mutex);
        pthread_mutex_destroy(&command_mutex);
    }
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
            if (targets[i] == player_num())
                found = qtrue;
        }
        if (!found)
            return;
    }
    cmd_execute(cmd);
}
