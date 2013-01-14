#include <stdio.h>
#include <stdlib.h>

#include "warsow.h"

int die(char *message) {
    printf("%s", message);
    exit(1);
}

void read_demo_metadata(FILE *fp) {
}

int read_byte(msg_t *message) {
    int i = (qbyte)message->data[message->readcount++];
    if (message->readcount > message->cursize)
        return -1;
    return i;
}

int read_short(msg_t *message)
{
	int i;
	short *sp = (short *)&message->data[message->readcount];
	i = LittleShort(*sp);
	message->readcount += 2;
	if( message->readcount > message->cursize )
		i = -1;
	return i;
}

int read_long(msg_t *message) {
	int i;
	unsigned int *ip = (unsigned int *)&message->data[message->readcount];
	i = LittleLong(*ip);
	message->readcount += 4;
	if (message->readcount > message->cursize)
		i = -1;
	return i;
}

char *read_string(msg_t *message) {
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

void read_data(msg_t *message, void *data, size_t length) {
	unsigned int i;
	for (i = 0; i < length; i++)
        ((qbyte *)data)[i] = read_byte(message);
}

int skip_data(msg_t *message, size_t length) {
    if (message->readcount + length <= message->cursize) {
        message->readcount += length;
        return 1;
    }
    return 0;
}

void parse_frame(msg_t *message) {
}

void parse_message(msg_t *message) {
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
            //case svc_spawnbaseline:
            //    parse_baseline(message);
            //    break;
            //case svc_frame:
            //    parse_frame(message);
            //    return;
            default:
                //printf("%d\n", cmd);
                return;
        }
    }
}

qboolean read_demo_message(FILE *fp) {
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

int main(int argc, char *argv[]) {
    if (argc < 2) {
        die("No demo given\n");
    }
    FILE *fp = fopen(argv[1], "r");
    read_demo_metadata(fp);
    while (read_demo_message(fp))
        ;
    fclose(fp);
    return 0;
}
