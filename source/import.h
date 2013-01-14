#ifndef WDP_WARSOW_H
#define WDP_WARSOW_H

#include <string.h>

#define MAX_MSGLEN 32768
#define MAX_MSG_STRING_CHARS 2048

#define MAX_EDICTS 1024

typedef unsigned char qbyte;
typedef enum {
    qfalse,
    qtrue
} qboolean;
typedef float vec_t;
typedef vec_t vec3_t[3];

typedef struct {
	qbyte data[MAX_MSGLEN];
	size_t maxsize;
	size_t cursize;
	size_t readcount;
	qboolean compressed;
} msg_t;

enum svc_ops_e {
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

#define PM_VECTOR_SNAP 16
#define   BYTE2ANGLE( x )         ( ( x )*( 360.0/256 ) )
#define   SHORT2ANGLE( x )        ( ( x )*( 360.0/65536 ) )

#define ET_INVERSE	128

typedef enum
{
	SOLID_NOT,				// no interaction with other objects
	SOLID_TRIGGER,			// only touch when inside, after moving
	SOLID_YES				// touch on edge
} solid_t;

#define SOLID_BMODEL	31	// special value for bmodel

#define EV_INVERSE	128

typedef struct entity_state_s {
	int number;							// edict index

	unsigned int svflags;

	int type;							// ET_GENERIC, ET_BEAM, etc
	qboolean linearProjectile;			// is sent inside "type" as ET_INVERSE flag
	vec3_t linearProjectileVelocity;	// this is transmitted instead of origin when linearProjectile is true

	vec3_t origin;
	vec3_t angles;

	union {
		vec3_t old_origin;				// for lerping
		vec3_t origin2;					// ET_BEAM, ET_PORTALSURFACE, ET_EVENT specific
	};

	unsigned int modelindex;
	union {
		unsigned int modelindex2;
		int bodyOwner;					// ET_PLAYER specific, for dead bodies
		int channel;					// ET_SOUNDEVENT
	};

	union {
		int frame;
		int ownerNum;					// ET_EVENT specific
	};

	union {
		int counterNum;					// ET_GENERIC
		int skinnum;					// for ET_PLAYER
		int itemNum;					// for ET_ITEM
		int firemode;					// for weapon events
		int damage;						// EV_BLOOD
		int targetNum;					// ET_EVENT specific
		int colorRGBA;					// ET_BEAM, ET_EVENT specific
		int range;						// ET_LASERBEAM, ET_CURVELASERBEAM specific
		int attenuation;				// ET_SOUNDEVENT
	};

	int weapon;							// WEAP_ for players
	qboolean teleported;				// the entity was teleported this snap (sent inside "weapon" as ET_INVERSE flag)

	unsigned int effects;

	union {
		// for client side prediction, 8*(bits 0-4) is x/y radius
		// 8*(bits 5-9) is z down distance, 8(bits10-15) is z up
		// GClip_LinkEntity sets this properly
		int solid;	
		int eventCount;					// ET_EVENT specific
	};

	int sound;							// for looping sounds, to guarantee shutoff

	// impulse events -- muzzle flashes, footsteps, etc
	// events only go out for a single frame, they
	// are automatically cleared each frame
	int events[2];
	int eventParms[2];

	union {
		unsigned int linearProjectileTimeStamp;
		int light;						// constant light glow
	};

	int team;							// team in the game
} entity_state_t;

// try to pack the common update flags into the first byte
#define	U_ORIGIN1	( 1<<0 )
#define	U_ORIGIN2	( 1<<1 )
#define	U_ORIGIN3	( 1<<2 )
#define	U_ANGLE1	( 1<<3 )
#define	U_ANGLE2	( 1<<4 )
#define	U_EVENT		( 1<<5 )
#define	U_REMOVE	( 1<<6 )      // REMOVE this entity, don't add it
#define	U_MOREBITS1	( 1<<7 )      // read one additional byte

// second byte
#define	U_NUMBER16	( 1<<8 )      // NUMBER8 is implicit if not set
#define	U_FRAME8	( 1<<9 )      // frame is a byte
#define	U_SVFLAGS	( 1<<10 )
#define	U_MODEL		( 1<<11 )
#define U_TYPE		( 1<<12 )
#define	U_OTHERORIGIN	( 1<<13 )     // FIXME: get rid of this
#define U_SKIN8		( 1<<14 )
#define	U_MOREBITS2	( 1<<15 )     // read one additional byte

// third byte
#define	U_EFFECTS8	( 1<<16 )     // autorotate, trails, etc
#define U_WEAPON	( 1<<17 )
#define	U_SOUND		( 1<<18 )
#define	U_MODEL2	( 1<<19 )     // weapons, flags, etc
#define U_LIGHT		( 1<<20 )
#define	U_SOLID		( 1<<21 )     // angles are short if bmodel (precise)
#define	U_EVENT2	( 1<<22 )
#define	U_MOREBITS3	( 1<<23 )     // read one additional byte

// fourth byte
#define	U_SKIN16	( 1<<24 )
#define	U_ANGLE3	( 1<<25 )     // for multiview, info need for culling
#define	U_____UNUSED1	( 1<<26 )
#define	U_EFFECTS16	( 1<<27 )
#define U_____UNUSED2	( 1<<28 )
#define	U_FRAME16	( 1<<29 )     // frame is a short
#define	U_TEAM		( 1<<30 )     // gameteam. Will rarely change

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
