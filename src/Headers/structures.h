//
// Structures.h
//

#pragma once


#include <Pomme.h>


#define	SF_HEADER__SHAPE_LIST	4
#define	SHAPE_HEADER_ANIM_LIST	6

#define	MAX_SHAPES_IN_FILE		100

#define	MAX_WEAPONS		50				// max weapons allowed in weapon list


typedef struct GamePalette_s
{
	uint32_t finalColors32[256];	// RGBA 8-8-8-8
	uint16_t finalColors16[256];	// RGB 5-6-5
	RGBColor baseColors[256];		// Mac RGB 16-16-16
} GamePalette;


			/* PLAYFIELD ITEM RECORD */

#pragma pack(push, 1)	// This struct must be tightly packed to match the on-disk representation.
						// (A static assert in the code enforces this.)

typedef struct ObjectEntryType
{
	int32_t			x;
	int32_t			y;
	int16_t			type;
	Byte			parm[4];
}
#if OSXPPC				// for some reason I can't get old GCC to honor #pragma pack on PPC
__attribute__((packed))
#endif
ObjectEntryType;

#pragma pack(pop)		// Stop tight packing of structs



typedef union MikeFixed
{
	int32_t			L;
	struct
	{
#if __BIG_ENDIAN__
		int16_t		Int;
		int16_t		Frac;
#else
		int16_t		Frac;
		int16_t		Int;
#endif
	};
} MikeFixed;


			/*  OBJECT RECORD STRUCTURE */


struct ObjNode
{
	long		Genre;			// obj genre: 0=sprite, 1=nonsprite
	unsigned  long	Z;			// z sort value
	long		Type;			// obj type
	long		SubType;		// sub type (anim type)
	long		SpriteGroupNum;	// sprite group # (if sprite genre)
	Boolean		DrawFlag;		// set if draw this object
	Boolean		EraseFlag;		// set if erase this object
	Boolean		UpdateBoxFlag;	// set if automatically make update region for shape
	Boolean		MoveFlag;		// set if move this object
	Boolean		AnimFlag;		// set if animate this object
	Boolean		PFCoordsFlag;	// set if x/y coords are global playfield coords, not offscreen buffer coords
	Boolean		TileMaskFlag;	// set if PF draw should use tile masks
	short		ClipNum;		// clipping region # to use
	MikeFixed	X;				// x coord (low word is fraction)
	MikeFixed	Y;				// y coord (low word is fraction)
	MikeFixed	YOffset;		// offset for y draw position on playfield
	MikeFixed	OldX;			// old x coord (low word is fraction)
	MikeFixed	OldY;			// old y coord (low word is fraction)
	MikeFixed	OldYOffset;		// old offset for y draw position on playfield
	Rect		drawBox;		// box obj was last drawn to

	long		DX;				// DX value (actually a fixed-point number)
    long		DY;				// DY value
    long		DZ;				// DZ value
	void		(*MoveCall)(void);	// pointer to object's move routine
	Ptr			AnimsList;		// ptr to object's animations list. nil = none
	long			AnimLine;		// line # in current anim
	long			CurrentFrame;	// current frame #
	unsigned long AnimConst;		// default "setspeed" rate
	long		AnimCount;		// current value of rate
	unsigned long AnimSpeed;		// amt to subtract from count/rate
	Boolean		Flag0;
	Boolean		Flag1;
	Boolean		Flag2;
	Boolean		Flag3;
	long		Special0;
	long		Special1;
	long		Special2;
	long		Special3;
	long		Misc1;
	struct ObjNode		*Ptr1;
	struct ObjNode		*MPlatform;
	unsigned long		CType;		// collision type bits
	unsigned long		CBits;		// collision attribute bits
	long			LeftSide;			// collision side coords
	long			RightSide;
	long			TopSide;
	long			BottomSide;
	long			TopOff;				// collision box side offsets
	long			BottomOff;
	long			LeftOff;
	long			RightOff;
	long			Kind;				// kind
	long			BaseX;
	long			BaseY;
	long			Health;				// health
	Ptr			SHAPE_HEADER_Ptr;	// addr of this sprite's SHAPE_HEADER (shape data must be completely byteswapped prior to setting in ObjNode!)
	long			OldLeftSide;
	long			OldRightSide;
	long			OldTopSide;
	long			OldBottomSide;
	ObjectEntryType *ItemIndex;		// pointer to item's spot in the ItemList
	struct ObjNode	*ShadowIndex;	// ptr to object's shadow or shadow's owner
	struct ObjNode  *OwnerToMessageNode;	// ptr to owner's message
	struct ObjNode  *MessageToOwnerNode;	// ptr to message's owner
	long			MessageTimer;		// time to display message

	long			Worth;				// "worth" of object / # coins to give
	long		InjuryThreshold;	// threshold for weapon to do damage to enemy

	long			NodeNum;			// node # in array (for internal use)
	struct ObjNode	*PrevNode;		// address of previous node in linked list
	struct ObjNode	*NextNode;		// address of next node in linked list
};
typedef struct ObjNode ObjNode;



					/* COLLISION STRUCTURES */
struct CollisionRec
{
	unsigned long	sides;
	unsigned long	type;
	ObjNode		*objectPtr;		// object that collides with (if object type)
};
typedef struct CollisionRec CollisionRec;




					/* WEAPON */

struct WeaponType
{
	uint8_t		type;
	uint16_t	life;
};
typedef struct WeaponType WeaponType;


					/* SAVED GAME */

struct SaveGameFile
{
	char		magic[32];
	WeaponType	myWeapons[MAX_WEAPONS];
	int32_t		score;
	int32_t		numCoins;
	int16_t		numLives;
	uint8_t		currentWeaponType;
	uint8_t		sceneNum;
	uint8_t		areaNum;
	uint8_t		numWeaponsIHave;
	uint8_t		currentWeaponIndex;
	int16_t		myHealth;
	int16_t		myMaxHealth;
	int8_t		difficultySetting;
};
typedef struct SaveGameFile SaveGameFile;

#define SAVEGAMEFILE_MAGIC "Mighty Mike Save v0"


			/* SAVED PLAYER INFO */

struct PlayerSaveType
{
	Boolean		beenSavedData;						// set if player data is saved on disk
	Boolean		newAreaFlag;						// set if last save was before going to new area
	Boolean		donePlayingFlag;					// set when this player is no longer in the game
	long		score,numCoins;
	short		lives;
	Byte		currentWeaponType;
	Byte		scene,area;
	short		numEnemies,numBunnies;
	short		myX,myY;
	short		nodeStackFront;						// copy of NodeStackFront
	short		numObjects;							// # objects in list
	ObjNode		*firstNodePtr,*myNodePtr;
	short		myBlinkieTimer;
	Byte		numItemsInInventory,inventoryIndex_weapon;
	Boolean		keys[6];
	short		myHealth,myMaxHealth;
	short		lastNonDeathX,lastNonDeathY;
	Ptr			oldItemIndex;
};
typedef struct PlayerSaveType PlayerSaveType;


			/* LONG PT */

struct LongPoint
{
	long		v,h;
};
typedef struct LongPoint LongPoint;


			/* INPUT NEEDS */

#define KEYBINDING_MAX_KEYS					2
#define KEYBINDING_MAX_GAMEPAD_BUTTONS		2

enum
{
	kNeed_Up,
	kNeed_Down,
	kNeed_Left,
	kNeed_Right,
	kNeed_Attack,
	kNeed_NextWeapon,
	kNeed_PrevWeapon,
	kNeed_Radar,
	kNeed_ToggleMusic,
	NUM_REMAPPABLE_NEEDS,

	// ^^^ REMAPPABLE
	// --------------------------------------------------------
	//              NON-REMAPPABLE vvv

	kNeed_UIUp = NUM_REMAPPABLE_NEEDS,
	kNeed_UIDown,
	kNeed_UILeft,
	kNeed_UIRight,
	kNeed_UIPrev,
	kNeed_UINext,
	kNeed_UIConfirm,
	kNeed_UIBack,
	kNeed_UIPause,
	NUM_CONTROL_NEEDS
};

enum
{
	kUnbound = 0,
	kButton,
	kAxisPlus,
	kAxisMinus,
};

typedef struct KeyBinding
{
	int16_t			key[KEYBINDING_MAX_KEYS];

	struct
	{
		int8_t		type;
		int8_t		id;
	} mouse;

	struct
	{
		int8_t		type;
		int8_t		id;
	} gamepad[KEYBINDING_MAX_GAMEPAD_BUTTONS];
} KeyBinding;

		/* SETTINGS */

struct PrefsType
{
	char		magic[32];
	Boolean		interlaceMode;
	Byte		difficulty;
	Byte		pfSize;
	Byte		displayMode;
	Byte		windowedZoom;
	Byte		preferredDisplay;
	Boolean		uncappedFramerate;
	Boolean		filterDithering;
	Boolean		music;
	Boolean		soundEffects;
	Boolean		interpolateAudio;
	Boolean		gameTitlePowerPete;
	Boolean		thermometerScreen;
	Boolean		debugInfoInTitleBar;
	Boolean		colorCorrection;
	KeyBinding	keys[NUM_CONTROL_NEEDS];
};
typedef struct PrefsType PrefsType;

#define PREFS_MAGIC "Mighty Mike Prefs v5"

