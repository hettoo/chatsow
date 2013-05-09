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

#ifndef WRLC_NET_H
#define WRLC_NET_H

#include <netinet/in.h>
#include <stdarg.h>

#include "import.h"

typedef struct sock_s {
    char *host;
    int port;

    qboolean connected;

    int sockfd;
    struct sockaddr_in serv_addr;
    socklen_t slen;

    int inseq;
    int outseq;
    qboolean sequenced;
    msg_t rmsg;
    msg_t smsg;

    int fragment_total;
    qbyte fragment_buffer[MAX_MSGLEN];
} sock_t;

void msg_clear(msg_t *msg);
void msg_copy(msg_t *msg, msg_t *source);
void write_string(msg_t *msg, const char *format, ...);
void vwrite_string(msg_t *msg, const char *format, va_list argptr);

void sock_init(sock_t *sock);
msg_t *sock_init_send(sock_t *sock, qboolean sequenced);
msg_t *sock_init_send_raw(sock_t *sock);
void sock_send(sock_t *sock);
void sock_connect(sock_t *sock, char *host, int port);
void sock_connect_tcp(sock_t *sock, char *host, int port);
void sock_disconnect(sock_t *sock);
msg_t *sock_recv(sock_t *sock);
msg_t *sock_recv_raw(sock_t *sock);

#endif
