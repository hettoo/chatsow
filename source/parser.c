#include <stdio.h>

#include "import.h"
#include "callbacks.h"

#define read_coord( sb ) ( (float)read_int3( ( sb ) )*( 1.0/PM_VECTOR_SNAP ) )
#define read_coord_dummy( sb ) ( read_int3( ( sb ) ) )
#define read_pos( sb, pos ) ( ( pos )[0] = read_coord( ( sb ) ), ( pos )[1] = read_coord( ( sb ) ), ( pos )[2] = read_coord( ( sb ) ) )
#define read_pos_dummy( sb, pos ) ( read_coord_dummy( ( sb ) ), read_coord_dummy( ( sb ) ), read_coord_dummy( ( sb ) ) )
#define read_angle( sb ) ( BYTE2ANGLE( read_byte( ( sb ) ) ) )
#define read_angle_dummy( sb ) ( read_byte( ( sb ) ) )
#define read_angle16( sb ) ( SHORT2ANGLE( read_short( ( sb ) ) ) )
#define read_angle16_dummy( sb ) ( read_short( ( sb ) ) )

static int last_frame = -1;

static int read_byte(msg_t *msg) {
    int i = (qbyte)msg->data[msg->readcount++];
    if (msg->readcount > msg->cursize)
        return -1;
    return i;
}

static int read_short(msg_t *msg)
{
	int i;
	short *sp = (short *)&msg->data[msg->readcount];
	i = LittleShort(*sp);
	msg->readcount += 2;
	if (msg->readcount > msg->cursize )
		i = -1;
	return i;
}

static int read_long(msg_t *msg) {
	int i;
	unsigned int *ip = (unsigned int *)&msg->data[msg->readcount];
	i = LittleLong(*ip);
	msg->readcount += 4;
	if (msg->readcount > msg->cursize)
		i = -1;
	return i;
}

static int read_int3(msg_t *msg)
{
	int i = msg->data[msg->readcount]
	| ( msg->data[msg->readcount+1]<<8 )
		| ( msg->data[msg->readcount+2]<<16 )
		| ( ( msg->data[msg->readcount+2] & 0x80 ) ? ~0xFFFFFF : 0 );
	msg->readcount += 3;
	if( msg->readcount > msg->cursize )
		i = -1;
	return i;
}

static char *read_string(msg_t *msg) {
	int l, c;
	static char string[MAX_MSG_STRING_CHARS];

	l = 0;
	do {
		c = read_byte(msg);
		if (c == -1 || c == '\0')
            break;

		string[l] = c;
		l++;
	} while ((unsigned int)l < sizeof(string) - 1);
	string[l] = '\0';

	return string;
}

static void read_data(msg_t *msg, void *data, size_t length) {
	unsigned int i;
	for (i = 0; i < length; i++)
        ((qbyte *)data)[i] = read_byte(msg);
}

unsigned read_entity_bits(msg_t *msg) {
	unsigned b, total;

	total = (qbyte)read_byte(msg);
	if (total & U_MOREBITS1) {
		b = (qbyte)read_byte(msg);
		total |= (b<<8)&0x0000FF00;
	}
	if (total & U_MOREBITS2) {
		b = (qbyte)read_byte(msg);
		total |= (b<<16)&0x00FF0000;
	}
	if (total & U_MOREBITS3) {
		b = (qbyte)read_byte(msg);
		total |= (b<<24)&0xFF000000;
	}

	if (total & U_NUMBER16)
		read_short(msg);
	else
		read_byte(msg);

    return total;
}

void read_delta_entity(msg_t *msg, unsigned bits) {
    qboolean linear_projectile = qfalse;
    short solid = 0;

	if( bits & U_TYPE )
        linear_projectile = ( read_byte( msg ) & ET_INVERSE ) ? qtrue : qfalse;

	if( bits & U_SOLID )
		solid = read_short( msg );

	if( bits & U_MODEL )
		read_byte( msg );
	if( bits & U_MODEL2 )
		read_byte( msg );

	if( bits & U_FRAME8 )
		read_byte( msg );
	if( bits & U_FRAME16 )
		read_short( msg );

	if( ( bits & U_SKIN8 ) && ( bits & U_SKIN16 ) )
		read_long( msg );
	else if( bits & U_SKIN8 )
		read_byte( msg );
	else if( bits & U_SKIN16 )
		read_short( msg );

	if( ( bits & ( U_EFFECTS8|U_EFFECTS16 ) ) == ( U_EFFECTS8|U_EFFECTS16 ) )
		read_long( msg );
	else if( bits & U_EFFECTS8 )
		read_byte( msg );
	else if( bits & U_EFFECTS16 )
		read_short( msg );

	if( linear_projectile ) {
		if( bits & U_ORIGIN1 )
			read_coord_dummy( msg );
		if( bits & U_ORIGIN2 )
			read_coord_dummy( msg );
		if( bits & U_ORIGIN3 )
			read_coord_dummy( msg );
	} else {
		if( bits & U_ORIGIN1 )
			read_coord_dummy( msg );
		if( bits & U_ORIGIN2 )
			read_coord_dummy( msg );
		if( bits & U_ORIGIN3 )
			read_coord_dummy( msg );
	}

	if( ( bits & U_ANGLE1 ) && ( solid == SOLID_BMODEL ) )
		read_angle16_dummy( msg );
	else if( bits & U_ANGLE1 )
		read_angle_dummy( msg );

	if( ( bits & U_ANGLE2 ) && ( solid == SOLID_BMODEL ) )
		read_angle16_dummy( msg );
	else if( bits & U_ANGLE2 )
		read_angle_dummy( msg );

	if( ( bits & U_ANGLE3 ) && ( solid == SOLID_BMODEL ) )
		read_angle16_dummy( msg );
	else if( bits & U_ANGLE3 )
		read_angle_dummy( msg );

	if( bits & U_OTHERORIGIN )
        read_pos_dummy(msg, NULL);

	if( bits & U_SOUND )
		read_byte( msg );

	if( bits & U_EVENT ) {
		int event = (qbyte)read_byte( msg );
		if( event & EV_INVERSE )
			read_byte( msg );
	}

	if( bits & U_EVENT2 ) {
		int event = (qbyte)read_byte( msg );
		if( event & EV_INVERSE )
			read_byte( msg );
	}

	if( bits & U_WEAPON )
		read_byte( msg );

	if( bits & U_SVFLAGS )
		read_short( msg );

	if( bits & U_LIGHT )
        read_long( msg );

	if( bits & U_TEAM )
		read_byte( msg );
}

static int skip_data(msg_t *msg, size_t length) {
    if (msg->readcount + length <= msg->cursize) {
        msg->readcount += length;
        return 1;
    }
    return 0;
}

static void parse_frame(msg_t *msg) {
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
        if (frame > last_frame + framediff)
            command(cmd, targets, numtargets);
    }
    skip_data(msg, length - (msg->readcount - pos));
    last_frame = frame;
}

static void parse_message(msg_t *msg) {
    int cmd;
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
                    demoinfo_key(read_string(msg));
                    demoinfo_value(read_string(msg));
                }
                //skip_data(msg, meta_data_realsize); // metadata
                skip_data(msg, meta_data_maxsize - meta_data_realsize);
                break;
            case svc_clcack:
                read_long(msg); // reliable acknownledge
                read_long(msg); // ucmd acknowledged
                break;
            case svc_servercmd:
                //read_long(msg); // command number, unreliable only
            case svc_servercs:
                command(read_string(msg), NULL, 0);
                break;
            case svc_serverdata:
                read_long(msg); // protocol version
                read_long(msg); // servercount
                read_short(msg); // snap frametime
                read_string(msg); // base game
                read_string(msg); // game
                read_short(msg); // player entity number
                read_string(msg); // level name
                read_byte(msg); // server bitflags
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
                parse_frame(msg);
                break;
            case -1:
                return;
            default:
                printf("unknown command: %d\n", cmd);
                return;
        }
    }
}

static qboolean read_demo_message(FILE *fp) {
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
    parse_message(&msg);
    return qtrue;
}

void parse_demo(FILE *fp) {
    while (read_demo_message(fp))
        ;
}
