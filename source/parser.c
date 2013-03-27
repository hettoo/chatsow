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

static void end_previous_demo(demo_t *demo) {
    long int backup = ftell(demo->fp);
    fseek(demo->fp, demo->start, SEEK_SET);
    int length = LittleLong(backup - demo->start - 4);
    fwrite(&length, 4, 1, demo->fp);
    fseek(demo->fp, backup, SEEK_SET);
}

FILE *parser_stop_record(parser_t *parser, int id) {
    demo_t *demo = parser->demos + id;
    FILE *result = demo->fp;
    if (result != NULL) {
        end_previous_demo(demo);
        demo->fp = NULL;
    }
    return result;
}

static void terminate_demos(parser_t *parser) {
    int i;
    for (i = 0; i < MAX_DEMOS; i++) {
        if (parser->demos[i].fp) {
            fclose(parser->demos[i].fp);
            parser_stop_record(parser, i);
        }
    }
}

void parser_reset(parser_t *parser) {
    static qboolean first = qtrue;
    parser->last_frame = -1;
    parser->last_cmd_num = 0;
    parser->last_cmd_ack = -1;
    if (!first)
        terminate_demos(parser);
    int i;
    for (i = 0; i < MAX_DEMOS; i++)
        parser->demos[i].fp = NULL;
    parser->initial.cursize = 0;
    for (i = 0; i < MAX_CLIENTS; i++)
        parser->playernums[i] = i;
    first = qfalse;
}

static void start_new_demo(demo_t *demo) {
    demo->start = ftell(demo->fp);
    int length = 0;
    fwrite(&length, 4, 1, demo->fp);
}

static void start_new(parser_t *parser) {
    int i;
    for (i = 0; i < MAX_DEMOS; i++) {
        demo_t *demo = parser->demos + i;
        if (demo->fp)
            start_new_demo(demo);
    }
}

static void end_previous(parser_t *parser) {
    int i;
    for (i = 0; i < MAX_DEMOS; i++) {
        demo_t *demo = parser->demos + i;
        if (demo->fp)
            end_previous_demo(demo);
    }
}

int parser_record(parser_t *parser, FILE *fp, int target) {
    int i;
    for (i = 0; i < MAX_DEMOS; i++) {
        demo_t *demo = parser->demos + i;
        if (demo->fp == NULL) {
            demo->fp = fp;
            demo->target = target;
            int x = 0;
            start_new_demo(demo);
            qbyte c = svc_demoinfo;
            char *key = "multipov";
            fwrite(&c, 1, 1, fp);
            x = LittleLong(20 + strlen(key) + 1 + 2);
            fwrite(&x, 4, 1, fp); // demoinfo length
            x = 4;
            fwrite(&x, 4, 1, fp); // metadata offset
            x = 0;
            fwrite(&x, 4, 1, fp); // time
            x = LittleLong(strlen(key) + 1 + 2);
            fwrite(&x, 4, 1, fp); // size
            fwrite(&x, 4, 1, fp); // max size
            fwrite(key, 1, strlen(key) + 1, fp);
            c = '0' + (target < 0);
            fwrite(&c, 1, 1, fp);
            c = 0;
            fwrite(&c, 1, 1, fp);
            end_previous_demo(demo);
            start_new_demo(demo);
            fwrite(parser->initial.data, 1, parser->initial.cursize, fp);
            int j;
            cs_t *cs = client_cs(parser->client);
            for (j = 0; j < MAX_CONFIGSTRINGS; j++) {
                char *string = cs_get(cs, j);
                if (string && string[0]) {
                    c = svc_servercs;
                    fwrite(&c, 1, 1, fp);
                    fprintf(fp, "cs %i \"%s\"", j, string);
                    c = 0;
                    fwrite(&c, 1, 1, fp);
                }
            }
            c = svc_servercs;
            fwrite(&c, 1, 1, fp);
            key = "precache";
            fwrite(key, 1, strlen(key) + 1, fp);
            demo->waiting = qtrue;
            client_command(parser->client, "nodelta");
            return i;
        }
    }
    return -1;
}

static void start_demos(parser_t *parser) {
    int i;
    for (i = 0; i < MAX_DEMOS; i++) {
        if (parser->demos[i].fp)
            parser->demos[i].waiting = qfalse;
    }
}

static void record_initial(parser_t *parser, msg_t *source, int size) {
    write_data(&parser->initial, source->data + source->readcount, size);
}

static qboolean target_match(int target, qbyte *targets) {
    if (target == -1 || targets == NULL)
        return qtrue;
    return (targets[target >> 3] & (1 << (target & 7))) > 0;
}

static void record(parser_t *parser, msg_t *msg, int size, qbyte *targets) {
    int i;
    for (i = 0; i < MAX_DEMOS; i++) {
        if (parser->demos[i].fp && !parser->demos[i].waiting && target_match(parser->demos[i].target, targets))
            fwrite(msg->data + msg->readcount, 1, size, parser->demos[i].fp);
    }
}

static void record_multipov(parser_t *parser, msg_t *msg, int size) {
    int i;
    for (i = 0; i < MAX_DEMOS; i++) {
        if (parser->demos[i].fp && !parser->demos[i].waiting && parser->demos[i].target == -1)
            fwrite(msg->data + msg->readcount, 1, size, parser->demos[i].fp);
    }
}

static void record_frameflags(parser_t *parser, msg_t *msg) {
    int i;
    qbyte b = msg->data[msg->readcount];
    for (i = 0; i < MAX_DEMOS; i++) {
        if (parser->demos[i].fp && !parser->demos[i].waiting) {
            if (parser->demos[i].target < 0)
                b |= FRAMESNAP_FLAG_MULTIPOV;
            else
                b &= ~FRAMESNAP_FLAG_MULTIPOV;
            fwrite(&b, 1, 1, parser->demos[i].fp);
        }
    }
}

static void record_last(parser_t *parser, msg_t *msg, qbyte *targets) {
    msg->readcount--;
    record(parser, msg, 1, targets);
    msg->readcount++;
}

static void prepare_fragment(parser_t *parser, msg_t *msg) {
    end_previous(parser);
    start_new(parser);
    record_last(parser, msg, NULL);
}

static void record_string(parser_t *parser, msg_t *msg, qbyte *targets) {
    int i;
    for (i = msg->readcount; msg->data[i]; i++)
        ;
    record(parser, msg, i - msg->readcount + 1, targets);
}

static int parse_player_state(msg_t *msg, int old) {
	int i, b;
    int result = old;

	int flags = read_byte(msg);
	if(flags & PS_MOREBITS1) {
		b = read_byte(msg);
		flags |= b<<8;
	}
	if( flags & PS_MOREBITS2 ) {
		b = read_byte( msg );
		flags |= b<<16;
	}
	if (flags & PS_MOREBITS3) {
		b = read_byte( msg );
		flags |= b<<24;
	}

	if( flags & PS_M_TYPE )
		read_byte(msg);

	if( flags & PS_M_ORIGIN0 )
        read_int3(msg);
	if( flags & PS_M_ORIGIN1 )
        read_int3(msg);
	if( flags & PS_M_ORIGIN2 )
        read_int3(msg);

	if( flags & PS_M_VELOCITY0 )
        read_int3(msg);
	if( flags & PS_M_VELOCITY1 )
        read_int3(msg);
	if( flags & PS_M_VELOCITY2 )
        read_int3(msg);

	if( flags & PS_M_TIME )
        read_byte(msg);

	if( flags & PS_M_FLAGS )
        read_short(msg);

	if( flags & PS_M_DELTA_ANGLES0 )
        read_short(msg);
	if( flags & PS_M_DELTA_ANGLES1 )
        read_short(msg);
	if( flags & PS_M_DELTA_ANGLES2 )
        read_short(msg);

	if( flags & PS_EVENT ) {
		int x = read_byte( msg );
		if( x & EV_INVERSE )
			read_byte( msg );
	}

	if( flags & PS_EVENT2 ) {
		int x = read_byte( msg );
		if( x & EV_INVERSE )
			read_byte( msg );
	}

	if( flags & PS_VIEWANGLES ) {
        read_short(msg);
        read_short(msg);
        read_short(msg);
	}

	if( flags & PS_M_GRAVITY )
        read_short(msg);

	if( flags & PS_WEAPONSTATE )
        read_byte(msg);

	if( flags & PS_FOV )
        read_byte(msg);

	if( flags & PS_POVNUM )
        read_byte(msg);

	if( flags & PS_PLAYERNUM )
		result = read_byte(msg);

	if( flags & PS_VIEWHEIGHT )
        read_char(msg);

	if( flags & PS_PMOVESTATS ) {
		int pmstatbits = read_short( msg );
		for( i = 0; i < PM_STAT_SIZE; i++ ) {
			if( pmstatbits & ( 1<<i ) )
				read_short(msg);
		}
	}

	if( flags & PS_INVENTORY ) {
		int invstatbits[SNAP_INVENTORY_LONGS];

		// parse inventory
		for( i = 0; i < SNAP_INVENTORY_LONGS; i++ )
			invstatbits[i] = read_long( msg );

		for( i = 0; i < MAX_ITEMS; i++ ) {
			if( invstatbits[i>>5] & ( 1<<(i&31) ) )
                read_byte(msg);
		}
	}

	if( flags & PS_PLRKEYS )
        read_byte(msg);

	int statbits[SNAP_STATS_LONGS];

	// parse stats
	for( i = 0; i < SNAP_STATS_LONGS; i++ )
		statbits[i] = read_long( msg );

	for( i = 0; i < PS_MAX_STATS; i++ ) {
		if( statbits[i>>5] & ( 1<<(i&31) ) )
			read_short( msg );
	}

    return result;
}

static void parse_delta_gamestate(msg_t *msg) {
	qbyte bits = read_byte( msg );
	short statbits = read_short( msg );

    if( bits ) {
        int i;
        for( i = 0; i < MAX_GAME_LONGSTATS; i++ ) {
            if( bits & ( 1<<i ) )
                read_long(msg);
        }
    }

    if( statbits ) {
        int i;
        for( i = 0; i < MAX_GAME_STATS; i++ ) {
            if( statbits & ( 1<<i ) )
                read_short(msg);
        }
    }
}

static void parse_frame(parser_t *parser, msg_t *msg) {
    int length = read_short(msg); // length
    int pos = msg->readcount;
    read_long(msg); // serverTime
    int frame = read_long(msg);
    read_long(msg); // delta frame number
    read_long(msg); // ucmd executed
    int flags = read_byte(msg);
    if (!(flags & FRAMESNAP_FLAG_DELTA)) {
        start_demos(parser);
        msg->readcount -= 1;
        msg->readcount -= 18;
        prepare_fragment(parser, msg);
        record(parser, msg, 18, NULL);
        msg->readcount += 18;
        record_frameflags(parser, msg);
        msg->readcount += 1;
    }
    record(parser, msg, 2, NULL);
    read_byte(msg); // suppresscount

    read_byte(msg); // svc_gamecommands
    int framediff;
    static qbyte targets[MAX_CLIENTS / 8];
    while ((framediff = read_short(msg)) != -1) {
        qboolean valid = frame > parser->last_frame + framediff;
        if (valid) {
            msg->readcount -= 2;
            record(parser, msg, 2, NULL);
            msg->readcount += 2;
        }
        int pos = msg->readcount;
        char *cmd = read_string(msg);
        int numtargets = 0;
        int i;
        for (i = 0; i < MAX_CLIENTS / 8; i++)
            targets[i] = 0;
        if (flags & FRAMESNAP_FLAG_MULTIPOV) {
            int a = msg->readcount;
            numtargets = read_byte(msg);
            int b = msg->readcount;
            read_data(msg, targets, numtargets);
            if (valid) {
                int real = msg->readcount;
                msg->readcount = pos;
                record_string(parser, msg, targets);
                msg->readcount = a;
                record_multipov(parser, msg, 1);
                msg->readcount = b;
                record_multipov(parser, msg, numtargets);
                msg->readcount = real;
            }
        } else {
            int real = msg->readcount;
            msg->readcount = pos;
            if (valid) {
                record_string(parser, msg, NULL);
                qbyte temp = msg->data[msg->readcount];
                msg->data[msg->readcount] = 0;
                record_multipov(parser, msg, 1);
                msg->data[msg->readcount] = temp;
            }
            msg->readcount = real;
        }
        if (valid)
            execute(parser->client, cmd, targets, numtargets);
    }
    msg->readcount -= 2;
    record(parser, msg, 2, NULL);
    msg->readcount += 2;

    record(parser, msg, 1, NULL);
    int bytes = read_byte(msg);
    record(parser, msg, bytes, NULL);
    skip_data(msg, bytes);

    int start = msg->readcount;
    read_byte(msg); // svc_match
    parse_delta_gamestate(msg);
    int backup = msg->readcount;
    msg->readcount = start;
    record(parser, msg, backup - start, NULL);
    msg->readcount = backup;

    int cmd;
    int players = 0;
    while ((cmd = read_byte(msg)) != 0) { // svc_playerinfo
        start = msg->readcount - 1;
        parser->playernums[players] = parse_player_state(msg, parser->playernums[players]);
        int i;
        for (i = 0; i < MAX_CLIENTS / 8; i++)
            targets[i] = 0;
        targets[parser->playernums[players] >> 3] |= 1 << (parser->playernums[players] & 7);
        backup = msg->readcount;
        msg->readcount = start;
        record(parser, msg, backup - start, targets);
        msg->readcount = backup;
        players++;
    }
    msg->readcount -= 1;
    record(parser, msg, 1, NULL);
    msg->readcount += 1;
    record(parser, msg, length - (msg->readcount - pos), NULL);
    skip_data(msg, length - (msg->readcount - pos));

    if (frame > parser->last_frame)
        client_ack_frame(parser->client, frame);

    parser->last_frame = frame;
}

void parse_message(parser_t *parser, msg_t *msg) {
    int cmd;
    int ack;
    int size;
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
                prepare_fragment(parser, msg);
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
                if (cmd == svc_servercs)
                    prepare_fragment(parser, msg);
                record_string(parser, msg, NULL);
                execute(parser->client, read_string(msg), NULL, 0);
                break;
            case svc_serverdata:
                size = msg->readcount - 1;
                set_protocol(parser->client, read_long(msg));
                set_spawn_count(parser->client, read_long(msg));
                read_short(msg); // snap frametime
                read_string(msg); // base game
                set_game(parser->client, read_string(msg));
                set_playernum(parser->client, read_short(msg) + 1);
                set_level(parser->client, read_string(msg)); // level name
                int flag_offset = msg->readcount;
                set_bitflags(parser->client, read_byte(msg));
                int pure = read_short(msg);
                while (pure > 0) {
                    read_string(msg); // pure pk3 name
                    read_long(msg); // checksum
                    pure--;
                }
                size = msg->readcount - size;
                msg->readcount -= size;
                msg->data[flag_offset] |= SV_BITFLAGS_RELIABLE;
                record_initial(parser, msg, size);
                msg->readcount += size;
                break;
            case svc_spawnbaseline:
                size = msg->readcount - 1;
                read_delta_entity(msg, read_entity_bits(msg));
                size = msg->readcount - size;
                msg->readcount -= size;
                record_initial(parser, msg, size);
                msg->readcount += size;
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
    static msg_t msg;
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
