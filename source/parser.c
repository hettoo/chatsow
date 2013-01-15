#include <stdio.h>

#include "import.h"
#include "game.h"
#include "callbacks.h"

#define read_coord( sb ) ( (float)read_int3( ( sb ) )*( 1.0/PM_VECTOR_SNAP ) )
#define read_pos( sb, pos ) ( ( pos )[0] = read_coord( ( sb ) ), ( pos )[1] = read_coord( ( sb ) ), ( pos )[2] = read_coord( ( sb ) ) )
#define read_angle( sb ) ( BYTE2ANGLE( read_byte( ( sb ) ) ) )
#define read_angle16( sb ) ( SHORT2ANGLE( read_short( ( sb ) ) ) )

static int last_frame = -1;

static int read_char(msg_t *msg) {
	int i = (signed char)msg->data[msg->readcount++];
	if(msg->readcount > msg->cursize)
		i = -1;
	return i;
}

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

int read_entity_bits(msg_t *msg, unsigned *bits) {
	unsigned b, total;
	int number;

	total = (qbyte)read_byte(msg);
	if( total & U_MOREBITS1 ) {
		b = (qbyte)read_byte(msg);
		total |= (b<<8)&0x0000FF00;
	}
	if( total & U_MOREBITS2 ) {
		b = (qbyte)read_byte(msg);
		total |= (b<<16)&0x00FF0000;
	}
	if( total & U_MOREBITS3 ) {
		b = (qbyte)read_byte(msg);
		total |= (b<<24)&0xFF000000;
	}

	if( total & U_NUMBER16 )
		number = read_short(msg);
	else
		number = (qbyte)read_byte(msg);

	*bits = total;

	return number;
}

void read_delta_entity(msg_t *msg, entity_state_t *from, entity_state_t *to, int number, unsigned bits) {
	// set everything to the state we are delta'ing from
	*to = *from;

	to->number = number;

	if( bits & U_TYPE )
	{
		qbyte ttype;
		ttype = (qbyte)read_byte( msg );
		to->type = ttype & ~ET_INVERSE;
		to->linearProjectile = ( ttype & ET_INVERSE ) ? qtrue : qfalse;
	}

	if( bits & U_SOLID )
		to->solid = read_short( msg );

	if( bits & U_MODEL )
		to->modelindex = (qbyte)read_byte( msg );
	if( bits & U_MODEL2 )
		to->modelindex2 = (qbyte)read_byte( msg );

	if( bits & U_FRAME8 )
		to->frame = (qbyte)read_byte( msg );
	if( bits & U_FRAME16 )
		to->frame = read_short( msg );

	if( ( bits & U_SKIN8 ) && ( bits & U_SKIN16 ) )  //used for laser colors
		to->skinnum = read_long( msg );
	else if( bits & U_SKIN8 )
		to->skinnum = read_byte( msg );
	else if( bits & U_SKIN16 )
		to->skinnum = read_short( msg );

	if( ( bits & ( U_EFFECTS8|U_EFFECTS16 ) ) == ( U_EFFECTS8|U_EFFECTS16 ) )
		to->effects = read_long( msg );
	else if( bits & U_EFFECTS8 )
		to->effects = (qbyte)read_byte( msg );
	else if( bits & U_EFFECTS16 )
		to->effects = read_short( msg );

	if( to->linearProjectile )
	{
		if( bits & U_ORIGIN1 )
			to->linearProjectileVelocity[0] = read_coord( msg );
		if( bits & U_ORIGIN2 )
			to->linearProjectileVelocity[1] = read_coord( msg );
		if( bits & U_ORIGIN3 )
			to->linearProjectileVelocity[2] = read_coord( msg );
	}
	else
	{
		if( bits & U_ORIGIN1 )
			to->origin[0] = read_coord( msg );
		if( bits & U_ORIGIN2 )
			to->origin[1] = read_coord( msg );
		if( bits & U_ORIGIN3 )
			to->origin[2] = read_coord( msg );
	}

	if( ( bits & U_ANGLE1 ) && ( to->solid == SOLID_BMODEL ) )
		to->angles[0] = read_angle16( msg );
	else if( bits & U_ANGLE1 )
		to->angles[0] = read_angle( msg );

	if( ( bits & U_ANGLE2 ) && ( to->solid == SOLID_BMODEL ) )
		to->angles[1] = read_angle16( msg );
	else if( bits & U_ANGLE2 )
		to->angles[1] = read_angle( msg );

	if( ( bits & U_ANGLE3 ) && ( to->solid == SOLID_BMODEL ) )
		to->angles[2] = read_angle16( msg );
	else if( bits & U_ANGLE3 )
		to->angles[2] = read_angle( msg );

	if( bits & U_OTHERORIGIN )
		read_pos( msg, to->origin2 );

	if( bits & U_SOUND )
		to->sound = (qbyte)read_byte( msg );

	if( bits & U_EVENT )
	{
		int event = (qbyte)read_byte( msg );
		if( event & EV_INVERSE )
			to->eventParms[0] = (qbyte)read_byte( msg );
		else
			to->eventParms[0] = 0;
		to->events[0] = ( event & ~EV_INVERSE );
	}
	else
	{
		to->events[0] = 0;
		to->eventParms[0] = 0;
	}

	if( bits & U_EVENT2 )
	{
		int event = (qbyte)read_byte( msg );
		if( event & EV_INVERSE )
			to->eventParms[1] = (qbyte)read_byte( msg );
		else
			to->eventParms[1] = 0;
		to->events[1] = ( event & ~EV_INVERSE );
	}
	else
	{
		to->events[1] = 0;
		to->eventParms[1] = 0;
	}

	if( bits & U_WEAPON )
	{
		qbyte tweapon;
		tweapon = (qbyte)read_byte( msg );
		to->weapon = tweapon & ~ET_INVERSE;
		to->teleported = ( tweapon & ET_INVERSE ) ? qtrue : qfalse;
	}

	if( bits & U_SVFLAGS )
		to->svflags = read_short( msg );

	if( bits & U_LIGHT )
	{
		if( to->linearProjectile )
			to->linearProjectileTimeStamp = (unsigned int)read_long( msg );
		else
			to->light = read_long( msg );
	}

	if( bits & U_TEAM )
		to->team = (qbyte)read_byte( msg );
}

static int skip_data(msg_t *msg, size_t length) {
    if (msg->readcount + length <= msg->cursize) {
        msg->readcount += length;
        return 1;
    }
    return 0;
}

static void parse_baseline(msg_t *msg) {
    unsigned bits;
    entity_state_t null_state;
	memset(&null_state, 0, sizeof(null_state));
    int num = read_entity_bits(msg, &bits);
    read_delta_entity(msg, &null_state, baselines + num, num, bits);
}

static void parse_playerstate(msg_t *msg) {
	int flags;
	int i, b;
	int statbits[SNAP_STATS_LONGS];

	flags = (qbyte)read_byte( msg );
	if( flags & PS_MOREBITS1 )
	{
		b = (qbyte)read_byte( msg );
		flags |= b<<8;
	}
	if( flags & PS_MOREBITS2 )
	{
		b = (qbyte)read_byte( msg );
		flags |= b<<16;
	}
	if( flags & PS_MOREBITS3 )
	{
		b = (qbyte)read_byte( msg );
		flags |= b<<24;
	}

	if( flags & PS_M_TYPE )
		read_byte( msg );

	if( flags & PS_M_ORIGIN0 )
		read_int3( msg );
	if( flags & PS_M_ORIGIN1 )
		read_int3( msg );
	if( flags & PS_M_ORIGIN2 )
		read_int3( msg );

	if( flags & PS_M_VELOCITY0 )
		read_int3( msg );
	if( flags & PS_M_VELOCITY1 )
		read_int3( msg );
	if( flags & PS_M_VELOCITY2 )
		read_int3( msg );

	if( flags & PS_M_TIME )
		read_byte( msg );

	if( flags & PS_M_FLAGS )
		read_short( msg );

	if( flags & PS_M_DELTA_ANGLES0 )
		read_short( msg );
	if( flags & PS_M_DELTA_ANGLES1 )
		read_short( msg );
	if( flags & PS_M_DELTA_ANGLES2 )
		read_short( msg );

    int event;
	if( flags & PS_EVENT )
	{
		event = read_byte( msg );
		if( event & EV_INVERSE )
			read_byte( msg );
	}

	if( flags & PS_EVENT2 )
	{
		event = read_byte( msg );
		if( event & EV_INVERSE )
			read_byte( msg );
	}

	if( flags & PS_VIEWANGLES )
	{
		//read_angle16( msg );
		//read_angle16( msg );
		//read_angle16( msg );
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
        read_byte(msg);

	if( flags & PS_VIEWHEIGHT )
        read_char(msg);

	if( flags & PS_PMOVESTATS )
	{
		int pmstatbits = read_short( msg );
		for( i = 0; i < PM_STAT_SIZE; i++ )
		{
			if( pmstatbits & ( 1<<i ) )
				read_short( msg );
		}
	}

	if( flags & PS_INVENTORY )
	{
		int invstatbits[SNAP_INVENTORY_LONGS];

		// parse inventory
		for( i = 0; i < SNAP_INVENTORY_LONGS; i++ ) {
			invstatbits[i] = read_long( msg );
		}

		for( i = 0; i < MAX_ITEMS; i++ )
		{
			if( invstatbits[i>>5] & ( 1<<(i&31) ) )
				read_byte( msg );
		}
	}

	if( flags & PS_PLRKEYS )
		read_byte( msg );

	// parse stats
	for( i = 0; i < SNAP_STATS_LONGS; i++ ) {
		read_long( msg );
	}

	for( i = 0; i < PS_MAX_STATS; i++ )
	{
		if( statbits[i>>5] & ( 1<<(i&31) ) )
			read_short( msg );
	}
}

static void parse_packet_entities(msg_t *msg) {
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
    /*
    skip_data(msg, read_byte(msg)); // areabits
    read_byte(msg); // svc_match
    int cmd;
    while ((cmd = read_byte(msg))) // svc_playerinfo
        parse_playerstate(msg);
    read_byte(msg); // svc_packetentities
    parse_packet_entities(msg);
    */
    last_frame = frame;
}

static void parse_message(msg_t *msg) {
    int cmd;
    while (1) {
        cmd = read_byte(msg);
        switch (cmd) {
            case svc_demoinfo:
                read_long(msg);
                read_long(msg);
                read_long(msg); // basetime
                size_t meta_data_realsize = read_long(msg);
                size_t meta_data_maxsize = read_long(msg);
                skip_data(msg, meta_data_realsize); // metadata
                skip_data(msg, meta_data_maxsize - meta_data_realsize);
                break;
            case svc_clcack:
                read_long(msg); // reliable acknownledge
                read_long(msg); // ucmd acknowledged
                break;
            case svc_servercmd:
                read_long(msg); // command number
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
            case svc_servercs:
                command(read_string(msg), NULL, 0);
                break;
            case svc_spawnbaseline:
                parse_baseline(msg);
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
