#ifndef WDP_WARSOW_H
#define WDP_WARSOW_H

#include <string.h>

#define MAX_MSGLEN 32768
#define MAX_MSG_STRING_CHARS 2048

typedef unsigned char qbyte;
typedef enum {
    qfalse,
    qtrue
} qboolean;

typedef struct {
	qbyte data[MAX_MSGLEN];
	size_t maxsize;
	size_t cursize;
	size_t readcount;
	qboolean compressed;
} msg_t;

enum svc_ops_e
{
	svc_bad,

	// the rest are private to the client and server
	svc_nop,
	svc_servercmd,          // [string] string
	svc_serverdata,         // [int] protocol ...
	svc_spawnbaseline,
	svc_download,           // [short] size [size bytes]
	svc_playerinfo,         // variable
	svc_packetentities,     // [...]
	svc_gamecommands,
	svc_match,
	svc_clcack,
	svc_servercs,			//tmp jalfixme : send reliable commands as unreliable
	svc_frame,
	svc_demoinfo,
	svc_extension			// for future expansion
};

#if !defined ( ENDIAN_LITTLE ) && !defined ( ENDIAN_BIG )
#if defined ( __i386__ ) || defined ( __ia64__ ) || defined ( WIN32 ) || ( defined ( __alpha__ ) || defined ( __alpha ) ) || defined ( __arm__ ) || ( defined ( __mips__ ) && defined ( __MIPSEL__ ) ) || defined ( __LITTLE_ENDIAN__ ) || defined ( __x86_64__ )
#define ENDIAN_LITTLE
#else
#define ENDIAN_BIG
#endif
#endif

short ShortSwap( short l );
int LongSwap( int l );
float FloatSwap( float f );

#ifdef ENDIAN_LITTLE
// little endian
#define BigShort( l ) ShortSwap( l )
#define LittleShort( l ) ( l )
#define BigLong( l ) LongSwap( l )
#define LittleLong( l ) ( l )
#define BigFloat( l ) FloatSwap( l )
#define LittleFloat( l ) ( l )
#elif defined ( ENDIAN_BIG )
// big endian
#define BigShort( l ) ( l )
#define LittleShort( l ) ShortSwap( l )
#define BigLong( l ) ( l )
#define LittleLong( l ) LongSwap( l )
#define BigFloat( l ) ( l )
#define LittleFloat( l ) FloatSwap( l )
#else
// figure it out at runtime
extern short ( *BigShort )(short l);
extern short ( *LittleShort )(short l);
extern int ( *BigLong )(int l);
extern int ( *LittleLong )(int l);
extern float ( *BigFloat )(float l);
extern float ( *LittleFloat )(float l);
#endif

void	Swap_Init( void );

#endif
