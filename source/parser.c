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

#include "import.h"
#include "client.h"
#include "ui.h"
#include "parser.h"

void parser_reset(parser_t *parser) {
    parser->last_frame = -1;
    parser->last_cmd_num = 0;
    parser->last_cmd_ack = -1;
}

static void parse_frame(parser_t *parser, msg_t *msg) {
    int length = read_short(msg); // length
    int pos = msg->readcount;
    read_long(msg); // serverTime
    int frame = read_long(msg);
    read_long(msg); // delta frame number
    read_long(msg); // ucmd executed
    int flags = read_byte(msg);
    read_byte(msg); // suppresscount

    read_byte(msg); // svc_gamecommands
    int framediff;
    while ((framediff = read_short(msg)) != -1) {
        char *cmd = read_string(msg);
        int numtargets = 0;
        static qbyte targets[MAX_CLIENTS / 8];
        if (flags & FRAMESNAP_FLAG_MULTIPOV) {
            numtargets = read_byte(msg);
            read_data(msg, targets, numtargets);
        }
        if (frame > parser->last_frame + framediff)
            execute(parser->client, cmd, targets, numtargets);
    }
    skip_data(msg, length - (msg->readcount - pos));
    parser->last_frame = frame;
}

void parse_message(parser_t *parser, msg_t *msg) {
    int cmd;
    int ack;
    while (1) {
        cmd = read_byte(msg);
        switch (cmd) {
            case svc_demoinfo:
                read_long(msg); // length
                read_long(msg); // meta data offset
                read_long(msg); // basetime
                size_t meta_data_realsize = read_long(msg);
                size_t meta_data_maxsize = read_long(msg);
                size_t end = msg->readcount + meta_data_realsize;
                while (msg->readcount < end) {
                    demoinfo_key(parser->client, read_string(msg));
                    demoinfo_value(parser->client, read_string(msg));
                }
                skip_data(msg, meta_data_maxsize - meta_data_realsize);
                break;
            case svc_clcack:
                ack = read_long(msg); // reliable ack
                if (ack > parser->last_cmd_ack) {
                    client_get_ack(parser->client, ack);
                    parser->last_cmd_ack = ack;
                }
                read_long(msg); // ucmd acknowledged
                client_activate(parser->client);
                break;
            case svc_servercmd:
                if (!(get_bitflags(parser->client) & SV_BITFLAGS_RELIABLE)) {
                    int cmd_num = read_long(msg);
                    if (cmd_num != parser->last_cmd_num + 1) {
                        read_string(msg);
                        break;
                    }
                    parser->last_cmd_num = cmd_num;
                    client_ack(parser->client, cmd_num);
                }
            case svc_servercs:
                execute(parser->client, read_string(msg), NULL, 0);
                break;
            case svc_serverdata:
                set_protocol(parser->client, read_long(msg));
                set_spawn_count(parser->client, read_long(msg));
                read_short(msg); // snap frametime
                read_string(msg); // base game
                set_game(parser->client, read_string(msg));
                set_playernum(parser->client, read_short(msg) + 1);
                set_level(parser->client, read_string(msg)); // level name
                set_bitflags(parser->client, read_byte(msg));
                int pure = read_short(msg);
                while (pure > 0) {
                    read_string(msg); // pure pk3 name
                    read_long(msg); // checksum
                    pure--;
                }
                break;
            case svc_spawnbaseline:
                read_delta_entity(msg, read_entity_bits(msg));
                break;
            case svc_frame:
                parse_frame(parser, msg);
                break;
            case -1:
                return;
            default:
                ui_output(parser->client, "Unknown command: %d\n", cmd);
                return;
        }
    }
}

static qboolean read_demo_message(parser_t *parser, FILE *fp) {
    int length;
    if (!fread(&length, 4, 1, fp))
        return qfalse;
    length = LittleLong(length);
    if (length < 0)
        return qfalse;
    msg_t msg;
    if (!fread(msg.data, length, 1, fp))
        return qfalse;
    msg.readcount = 0;
    msg.cursize = length;
    msg.maxsize = sizeof(msg.data);
    msg.compressed = qfalse;
    parse_message(parser, &msg);
    return qtrue;
}

void parse_demo(parser_t *parser, FILE *fp) {
    while (read_demo_message(parser, fp))
        ;
}
