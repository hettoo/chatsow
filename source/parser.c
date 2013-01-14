#include <stdio.h>

#include "import.h"
#include "game.h"

#define read_coord( sb ) ( (float)read_int3( ( sb ) )*( 1.0/PM_VECTOR_SNAP ) )
#define read_pos( sb, pos ) ( ( pos )[0] = read_coord( ( sb ) ), ( pos )[1] = read_coord( ( sb ) ), ( pos )[2] = read_coord( ( sb ) ) )
#define read_angle( sb ) ( BYTE2ANGLE( read_byte( ( sb ) ) ) )
#define read_angle16( sb ) ( SHORT2ANGLE( read_short( ( sb ) ) ) )

static void read_demo_metadata(FILE *fp) {
}

static int read_byte(msg_t *message) {
    int i = (qbyte)message->data[message->readcount++];
    if (message->readcount > message->cursize)
        return -1;
    return i;
}

static int read_short(msg_t *message)
{
	int i;
	short *sp = (short *)&message->data[message->readcount];
	i = LittleShort(*sp);
	message->readcount += 2;
	if (message->readcount > message->cursize )
		i = -1;
	return i;
}

static int read_long(msg_t *message) {
	int i;
	unsigned int *ip = (unsigned int *)&message->data[message->readcount];
	i = LittleLong(*ip);
	message->readcount += 4;
	if (message->readcount > message->cursize)
		i = -1;
	return i;
}

static int read_int3(msg_t *message)
{
	int i = message->data[message->readcount]
	| ( message->data[message->readcount+1]<<8 )
		| ( message->data[message->readcount+2]<<16 )
		| ( ( message->data[message->readcount+2] & 0x80 ) ? ~0xFFFFFF : 0 );
	message->readcount += 3;
	if( message->readcount > message->cursize )
		i = -1;
	return i;
}

static char *read_string(msg_t *message) {
	int l, c;
	static char string[MAX_MSG_STRING_CHARS];

	l = 0;
	do {
		c = read_byte(message);
		if (c == -1 || c == '\0')
            break;

		string[l] = c;
		l++;
	} while ((unsigned int)l < sizeof(string) - 1);
	string[l] = '\0';

	return string;
}

static void read_data(msg_t *message, void *data, size_t length) {
	unsigned int i;
	for (i = 0; i < length; i++)
        ((qbyte *)data)[i] = read_byte(message);
}

int read_entity_bits(msg_t *message, unsigned *bits) {
	unsigned b, total;
	int number;

	total = (qbyte)read_byte(message);
	if( total & U_MOREBITS1 ) {
		b = (qbyte)read_byte(message);
		total |= (b<<8)&0x0000FF00;
	}
	if( total & U_MOREBITS2 ) {
		b = (qbyte)read_byte(message);
		total |= (b<<16)&0x00FF0000;
	}
	if( total & U_MOREBITS3 ) {
		b = (qbyte)read_byte(message);
		total |= (b<<24)&0xFF000000;
	}

	if( total & U_NUMBER16 )
		number = read_short(message);
	else
		number = (qbyte)read_byte(message);

	*bits = total;

	return number;
}

void read_delta_entity(msg_t *message, entity_state_t *from, entity_state_t *to, int number, unsigned bits) {
	// set everything to the state we are delta'ing from
	*to = *from;

	to->number = number;

	if( bits & U_TYPE )
	{
		qbyte ttype;
		ttype = (qbyte)read_byte( message );
		to->type = ttype & ~ET_INVERSE;
		to->linearProjectile = ( ttype & ET_INVERSE ) ? qtrue : qfalse;
	}

	if( bits & U_SOLID )
		to->solid = read_short( message );

	if( bits & U_MODEL )
		to->modelindex = (qbyte)read_byte( message );
	if( bits & U_MODEL2 )
		to->modelindex2 = (qbyte)read_byte( message );

	if( bits & U_FRAME8 )
		to->frame = (qbyte)read_byte( message );
	if( bits & U_FRAME16 )
		to->frame = read_short( message );

	if( ( bits & U_SKIN8 ) && ( bits & U_SKIN16 ) )  //used for laser colors
		to->skinnum = read_long( message );
	else if( bits & U_SKIN8 )
		to->skinnum = read_byte( message );
	else if( bits & U_SKIN16 )
		to->skinnum = read_short( message );

	if( ( bits & ( U_EFFECTS8|U_EFFECTS16 ) ) == ( U_EFFECTS8|U_EFFECTS16 ) )
		to->effects = read_long( message );
	else if( bits & U_EFFECTS8 )
		to->effects = (qbyte)read_byte( message );
	else if( bits & U_EFFECTS16 )
		to->effects = read_short( message );

	if( to->linearProjectile )
	{
		if( bits & U_ORIGIN1 )
			to->linearProjectileVelocity[0] = read_coord( message );
		if( bits & U_ORIGIN2 )
			to->linearProjectileVelocity[1] = read_coord( message );
		if( bits & U_ORIGIN3 )
			to->linearProjectileVelocity[2] = read_coord( message );
	}
	else
	{
		if( bits & U_ORIGIN1 )
			to->origin[0] = read_coord( message );
		if( bits & U_ORIGIN2 )
			to->origin[1] = read_coord( message );
		if( bits & U_ORIGIN3 )
			to->origin[2] = read_coord( message );
	}

	if( ( bits & U_ANGLE1 ) && ( to->solid == SOLID_BMODEL ) )
		to->angles[0] = read_angle16( message );
	else if( bits & U_ANGLE1 )
		to->angles[0] = read_angle( message );

	if( ( bits & U_ANGLE2 ) && ( to->solid == SOLID_BMODEL ) )
		to->angles[1] = read_angle16( message );
	else if( bits & U_ANGLE2 )
		to->angles[1] = read_angle( message );

	if( ( bits & U_ANGLE3 ) && ( to->solid == SOLID_BMODEL ) )
		to->angles[2] = read_angle16( message );
	else if( bits & U_ANGLE3 )
		to->angles[2] = read_angle( message );

	if( bits & U_OTHERORIGIN )
		read_pos( message, to->origin2 );

	if( bits & U_SOUND )
		to->sound = (qbyte)read_byte( message );

	if( bits & U_EVENT )
	{
		int event = (qbyte)read_byte( message );
		if( event & EV_INVERSE )
			to->eventParms[0] = (qbyte)read_byte( message );
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
		int event = (qbyte)read_byte( message );
		if( event & EV_INVERSE )
			to->eventParms[1] = (qbyte)read_byte( message );
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
		tweapon = (qbyte)read_byte( message );
		to->weapon = tweapon & ~ET_INVERSE;
		to->teleported = ( tweapon & ET_INVERSE ) ? qtrue : qfalse;
	}

	if( bits & U_SVFLAGS )
		to->svflags = read_short( message );

	if( bits & U_LIGHT )
	{
		if( to->linearProjectile )
			to->linearProjectileTimeStamp = (unsigned int)read_long( message );
		else
			to->light = read_long( message );
	}

	if( bits & U_TEAM )
		to->team = (qbyte)read_byte( message );
}
static int skip_data(msg_t *message, size_t length) {
    if (message->readcount + length <= message->cursize) {
        message->readcount += length;
        return 1;
    }
    return 0;
}

static void parse_baseline(msg_t *message) {
    unsigned bits;
    entity_state_t null_state;
	memset(&null_state, 0, sizeof(null_state));
    int num = read_entity_bits(message, &bits);
    read_delta_entity(message, &null_state, baselines + num, num, bits);
}

static void parse_frame(msg_t *message) {
}

static void parse_message(msg_t *message) {
    int cmd;
    while (1) {
        cmd = read_byte(message);
        switch (cmd) {
            case svc_demoinfo:
                read_long(message);
                read_long(message);
                read_long(message); // basetime
                size_t meta_data_realsize = read_long(message);
                size_t meta_data_maxsize = read_long(message);
                skip_data(message, meta_data_realsize); // metadata
                skip_data(message, meta_data_maxsize - meta_data_realsize);
                break;
            case svc_clcack:
                read_long(message); // reliable acknownledge
                read_long(message); // ucmd acknowledged
                break;
            case svc_servercmd:
                read_long(message); // command number
                read_string(message); // command
                break;
            case svc_serverdata:
                read_long(message); // protocol version
                read_long(message); // servercount
                read_short(message); // snap frametime
                read_string(message); // base game
                read_string(message); // game
                read_short(message); // player entity number
                read_string(message); // level name
                read_byte(message); // server bitflags
                int pure = read_short(message);
                while (pure > 0) {
                    read_string(message); // pure pk3 name
                    read_long(message); // checksum
                    pure--;
                }
                break;
            case svc_servercs:
                read_string(message); // server command
                break;
            case svc_spawnbaseline:
                parse_baseline(message);
                break;
            //case svc_frame:
            //    parse_frame(message);
            //    break;
            default:
                //printf("%d\n", cmd);
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
    msg_t message;
    if (!fread(message.data, length, 1, fp))
        return qfalse;
    message.readcount = 0;
    message.cursize = length;
    message.maxsize = sizeof(message.data);
    message.compressed = qfalse;
    parse_message(&message);
    return qtrue;
}

void parse_demo(FILE *fp) {
    read_demo_metadata(fp);
    while (read_demo_message(fp))
        ;
}
