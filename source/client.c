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
#include <stdlib.h>
#include <string.h>

#include "import.h"
#include "utils.h"
#include "ui.h"
#include "parser.h"

static qboolean started = qfalse;
static pthread_t thread;

static char *host;
static struct sockaddr_in serv_addr;
static int sockfd;
static socklen_t slen = sizeof(serv_addr);

static void msg_clear(msg_t *msg) {
    msg->readcount = 0;
    msg->cursize = 0;
    msg->maxsize = sizeof(msg->data);
}

static void client_connect() {
    if ((sockfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1)
        die("socket");

    bzero(&serv_addr, sizeof(serv_addr));

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(44400);

    if (inet_aton(host, &serv_addr.sin_addr) == 0)
        die("inet_aton() failed\n");

}

static void client_send(msg_t *msg) {
    if (sendto(sockfd, msg->data, msg->cursize, 0, (struct sockaddr*)&serv_addr, slen) == -1)
        die("sendto");
}

static void client_recv(msg_t *msg) {
    msg->readcount = 0;
    msg->maxsize = sizeof(msg->data);
    msg->compressed = qfalse;
    if ((msg->cursize = recvfrom(sockfd, msg->data, msg->maxsize, 0, (struct sockaddr*)&serv_addr, &slen)) == -1)
        die("recvfrom()");
}

static void *client_run(void *args) {
    client_connect();

    msg_t msg;
    msg_clear(&msg);
    write_long(&msg, -1);
    write_string(&msg, "getchallenge");
    client_send(&msg);

    client_recv(&msg);
    printf("%s\n", msg.data);

    msg_clear(&msg);
    write_long(&msg, -1);
    write_string(&msg, "connect 15 44400 16675 \"\" 0\n");
    client_send(&msg);

    while (1) {
        client_recv(&msg);
        printf("%s\n", msg.data);
        parse_message(&msg);
    }

    close(sockfd);

    return NULL;
}

void client_start(char *new_host) {
    host = new_host;
    pthread_create(&thread, NULL, client_run, NULL);
    started = qtrue;
}

void client_stop() {
    if (started) {
        pthread_join(thread, NULL);
        started = qfalse;
    }
}

void execute(char *command) {
    ui_output(command);
}
