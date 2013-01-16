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

#define   PS_MAX_STATS                    64

#define FRAMESNAP_FLAG_DELTA		( 1<<0 )
#define FRAMESNAP_FLAG_ALLENTITIES	( 1<<1 )
#define FRAMESNAP_FLAG_MULTIPOV		( 1<<2 )

#define	PS_M_TYPE	    ( 1<<0 )
#define	PS_M_ORIGIN0	( 1<<1 )
#define	PS_M_ORIGIN1	( 1<<2 )
#define	PS_M_ORIGIN2	( 1<<3 )
#define	PS_M_VELOCITY0	( 1<<4 )
#define	PS_M_VELOCITY1	( 1<<5 )
#define	PS_M_VELOCITY2	( 1<<6 )
#define PS_MOREBITS1	( 1<<7 )

#define	PS_M_TIME	    ( 1<<8 )
#define	PS_EVENT	    ( 1<<9 )
#define	PS_EVENT2	    ( 1<<10 )
#define	PS_WEAPONSTATE	( 1<<11 )
#define PS_INVENTORY	( 1<<12 )
#define	PS_FOV		    ( 1<<13 )
#define	PS_VIEWANGLES	( 1<<14 )
#define PS_MOREBITS2	( 1<<15 )

#define	PS_POVNUM	    ( 1<<16 )
#define	PS_VIEWHEIGHT	( 1<<17 )
#define PS_PMOVESTATS	( 1<<18 )
#define	PS_M_FLAGS	    ( 1<<19 )
#define PS_PLRKEYS	    ( 1<<20 )
//...
#define PS_MOREBITS3	( 1<<23 )

#define	PS_M_GRAVITY	    ( 1<<24 )
#define	PS_M_DELTA_ANGLES0  ( 1<<25 )
#define	PS_M_DELTA_ANGLES1  ( 1<<26 )
#define	PS_M_DELTA_ANGLES2  ( 1<<27 )
#define	PS_PLAYERNUM	    ( 1<<28 )

#define PM_VECTOR_SNAP 16
#define MAX_PM_STATS 16

enum
{
	PM_STAT_FEATURES,
	PM_STAT_NOUSERCONTROL,
	PM_STAT_KNOCKBACK,
	PM_STAT_CROUCHTIME,
	PM_STAT_ZOOMTIME,
	PM_STAT_DASHTIME,
	PM_STAT_WJTIME,
	PM_STAT_NOAUTOATTACK,
	PM_STAT_STUN,
	PM_STAT_MAXSPEED,
	PM_STAT_JUMPSPEED,
	PM_STAT_DASHSPEED,
	PM_STAT_FWDTIME,

	PM_STAT_SIZE = MAX_PM_STATS
};

#define   BYTE2ANGLE( x )         ( ( x )*( 360.0/256 ) )
#define   SHORT2ANGLE( x )        ( ( x )*( 360.0/65536 ) )

#define CS_HOSTNAME			0
#define CS_TVSERVER			1
#define	CS_MAXCLIENTS		2
#define CS_MODMANIFEST		3

#define SERVER_PROTECTED_CONFIGSTRINGS 5

#define	CS_MESSAGE			5
#define	CS_MAPNAME			6
#define	CS_AUDIOTRACK		7
#define CS_SKYBOX			8
#define CS_STATNUMS			9
#define CS_POWERUPEFFECTS	10
#define CS_GAMETYPETITLE	11
#define CS_GAMETYPENAME		12
#define CS_GAMETYPEVERSION	13
#define CS_GAMETYPEAUTHOR	14
#define CS_AUTORECORDSTATE	15

#define CS_SCB_PLAYERTAB_LAYOUT 16
#define CS_SCB_PLAYERTAB_TITLES 17

#define CS_TEAM_SPECTATOR_NAME 18
#define CS_TEAM_PLAYERS_NAME 19
#define CS_TEAM_ALPHA_NAME	20
#define CS_TEAM_BETA_NAME	21

#define CS_MATCHNAME		22
#define CS_MATCHSCORE		23
#define CS_MATCHUUID		24

#define CS_WORLDMODEL		30
#define	CS_MAPCHECKSUM		31		// for catching cheater maps

//precache stuff begins here
#define	CS_MODELS			32
#define	CS_SOUNDS			( CS_MODELS+MAX_MODELS )
#define	CS_IMAGES			( CS_SOUNDS+MAX_SOUNDS )
#define	CS_SKINFILES		( CS_IMAGES+MAX_IMAGES )
#define	CS_LIGHTS			( CS_SKINFILES+MAX_SKINFILES )
#define	CS_ITEMS			( CS_LIGHTS+MAX_LIGHTSTYLES )
#define	CS_PLAYERINFOS		( CS_ITEMS+MAX_ITEMS )
#define CS_GAMECOMMANDS		( CS_PLAYERINFOS+MAX_CLIENTS )
#define CS_LOCATIONS		( CS_GAMECOMMANDS+MAX_GAMECOMMANDS )
#define CS_WEAPONDEFS		( CS_LOCATIONS+MAX_LOCATIONS )
#define CS_GENERAL			( CS_WEAPONDEFS+MAX_WEAPONDEFS )

#define	MAX_CONFIGSTRINGS	( CS_GENERAL+MAX_GENERAL )

#define	MAX_CLIENTS					256			// absolute limit
#define	MAX_EDICTS					1024		// must change protocol to increase more
#define	MAX_LIGHTSTYLES				256
#define	MAX_MODELS					256			// these are sent over the net as bytes
#define	MAX_SOUNDS					256			// so they cannot be blindly increased
#define	MAX_IMAGES					256
#define MAX_SKINFILES				256
#define MAX_ITEMS					64			// 16x4
#define MAX_GENERAL					( MAX_CLIENTS )	// general config strings

#define	MAX_GAME_STATS	16
#define MAX_GAME_LONGSTATS 8

#define SNAP_INVENTORY_LONGS			((MAX_ITEMS + 31) / 32)
#define SNAP_STATS_LONGS				((PS_MAX_STATS + 31) / 32)

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
