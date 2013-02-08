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
#include <errno.h>
#include <stdio.h>
#include "zlib.h"

#include "global.h"
#include "utils.h"
#include "ui.h"
#include "net.h"

void msg_clear(msg_t *msg) {
    msg->readcount = 0;
    msg->cursize = 0;
    msg->maxsize = sizeof(msg->data);
    msg->compressed = qfalse;
}

void vwrite_string(msg_t *msg, const char *format, va_list argptr) {
    static char string[MAX_MSGLEN];
    int len = vsprintf(string, format, argptr);
    write_data(msg, string, len + 1);
}

void write_string(msg_t *msg, const char *format, ...) {
	if (!format) {
		write_char(msg, '\0');
        return;
    }
	va_list	argptr;
	va_start(argptr, format);
    vwrite_string(msg, format, argptr);
	va_end(argptr);
}

void sock_init(sock_t *sock) {
    sock->connected = qfalse;
    sock->slen = sizeof(sock->serv_addr);
}

msg_t *sock_init_send(sock_t *sock, qboolean sequenced) {
    sock->sequenced = sequenced;
    msg_clear(&sock->smsg);
    if (sequenced) {
        write_long(&sock->smsg, sock->outseq++);
        write_long(&sock->smsg, sock->inseq);
        write_short(&sock->smsg, sock->port);
    } else {
        write_long(&sock->smsg, -1);
    }
    return &sock->smsg;
}

void sock_send(sock_t *sock) {
    if (!sock->connected)
        return;

    if (sendto(sock->sockfd, sock->smsg.data, sock->smsg.cursize, 0, (struct sockaddr*)&sock->serv_addr, sock->slen) == -1)
        die("sendto failed");
}

void sock_connect(sock_t *sock, char *host, int port) {
    sock->outseq = 1;
    sock->inseq = 0;

    sock->host = host;
    sock->port = port;

    if ((sock->sockfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1) {
        ui_output(-2, "Unable to create socket.\n");
        return;
    }

    bzero(&sock->serv_addr, sock->slen);

    sock->serv_addr.sin_family = AF_INET;
    sock->serv_addr.sin_port = htons(port);

    if (inet_aton(host, &sock->serv_addr.sin_addr) == 0) {
        ui_output(-2, "Invalid hostname.\n");
        sock->sockfd = -1;
        return;
    }
    sock->connected = qtrue;
}

void sock_disconnect(sock_t *sock) {
    if (!sock->connected)
        return;

    sock->connected = qfalse;
    close(sock->sockfd);
}

msg_t *sock_recv(sock_t *sock) {
    if (!sock->connected)
        return NULL;

    msg_clear(&sock->rmsg);
    if ((sock->rmsg.cursize = recvfrom(sock->sockfd, sock->rmsg.data, sock->rmsg.maxsize, MSG_DONTWAIT, (struct sockaddr*)&sock->serv_addr, &sock->slen)) == -1) {
        if (errno == EAGAIN)
            return NULL;
        die("recvfrom failed");
    }
    int seq = read_long(&sock->rmsg);
    if (seq == -1) {
        sock->sequenced = qfalse;
        return &sock->rmsg;
    }
    sock->sequenced = qtrue;
    qboolean compressed = qfalse;
    qboolean fragmented = qfalse;
    if (seq & FRAGMENT_BIT) {
        seq &= ~FRAGMENT_BIT;
        fragmented = qtrue;
    }
    // TODO: prefer order above quick messages and especially dropping like we
    // do now, so buffer out of order future messages
    if (seq < sock->inseq || (!fragmented && seq == sock->inseq))
        return NULL;
    sock->inseq = seq;
    int ack = read_long(&sock->rmsg);
    if (ack & FRAGMENT_BIT) {
        ack &= ~FRAGMENT_BIT;
        compressed = qtrue;
    }
    // TODO: buffer sent messages and timeout acks
    if (fragmented) {
        short fragment_start = read_short(&sock->rmsg);
        short fragment_length = read_short(&sock->rmsg);
        if (fragment_start != sock->fragment_total) {
            sock->rmsg.readcount = 0;
            sock->rmsg.cursize = 0;
            return NULL;
        }
        qboolean last = qfalse;
        if (fragment_length & FRAGMENT_LAST) {
            fragment_length &= ~FRAGMENT_LAST;
            last = qtrue;
        }
        read_data(&sock->rmsg, sock->fragment_buffer + sock->fragment_total, fragment_length);
        sock->fragment_total += fragment_length;
        sock->rmsg.readcount = 0;
        if (last) {
            memcpy(sock->rmsg.data, sock->fragment_buffer, sock->fragment_total);
            sock->rmsg.cursize = sock->fragment_total;
            sock->fragment_total = 0;
        } else {
            sock->rmsg.cursize = 0;
            return NULL;
        }
    }
    if (compressed && sock->rmsg.cursize - sock->rmsg.readcount > 0) {
        static qbyte temp[MAX_MSGLEN];
        unsigned long new_size = MAX_MSGLEN;
        uncompress(temp, &new_size, sock->rmsg.data + sock->rmsg.readcount, sock->rmsg.cursize - sock->rmsg.readcount);
        memcpy(sock->rmsg.data, temp, new_size);
        sock->rmsg.readcount = 0;
        sock->rmsg.cursize = new_size;
    }
    return &sock->rmsg;
}
