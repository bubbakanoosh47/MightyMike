/****************************/
/*   PLAYFIELD ROUTINES     */
/* (c)1994 Pangea Software  */
/*   By Brian Greenstone    */
/****************************/


/***************/
/* EXTERNALS   */
/***************/

#include "myglobals.h"
#include "window.h"
#include "playfield.h"
#include "object.h"
#include "enemy.h"
#include "traps.h"
#include "triggers.h"
#include "bonus.h"
#include "misc.h"
#include "miscanims.h"
#include "objecttypes.h"
#include "weapon.h"
#include "enemy2.h"
#include "enemy3.h"
#include "enemy4.h"
#include "enemy5.h"
#include "racecar.h"
#include "externs.h"
#include <string.h>

/****************************/
/*    CONSTANTS             */
/****************************/

											// FOR PPC, THESE ARE VARS SO WE CAN CHANGE THEM!
long	PF_TILE_HEIGHT	=	13;				// dimensions of scrolling Playfield
long	PF_TILE_WIDTH	=	14;
long	PF_WINDOW_TOP	=	45;
long	PF_WINDOW_LEFT	=	24;				// left MUST be on 4 pixel boundary!!!!!

#define	ITEM_WINDOW_RIGHT		5				// # tiles for item add window
#define	ITEM_WINDOW_LEFT		5
#define	ITEM_WINDOW_TOP			5
#define	ITEM_WINDOW_BOTTOM		5
#define	OUTER_SIZE				7				// size of border out of add window for delete window


#define SCROLL_WINDOW_XMARGIN	((PF_WINDOW_WIDTH/2)-40)
#define SCROLL_WINDOW_YMARGIN	((PF_WINDOW_HEIGHT/2)-40)
#define	SCROLL_WINDOW_LEFT		SCROLL_WINDOW_XMARGIN				// window for myguy's scroll movement
#define	SCROLL_WINDOW_RIGHT		(PF_WINDOW_WIDTH-SCROLL_WINDOW_XMARGIN)
#define	SCROLL_WINDOW_TOP		(SCROLL_WINDOW_YMARGIN+40)
#define	SCROLL_WINDOW_BOTTOM	(PF_WINDOW_HEIGHT-SCROLL_WINDOW_YMARGIN)

#define	SCROLL_BORDER			(TILE_SIZE*2L)					// 2 tile border around entire map

#define	VIEW_FACTOR		100				// amount to shift view for look-space

#define	MAX_PLAYFIELD_WIDTH	1000L		// max tiles wide the PF will ever be

#define	MAX_TILE_ANIMS	50						// max # of tile anims




/**********************/
/*     VARIABLES      */
/**********************/

static	Handle			gTileSetHandle = nil;
static	Ptr				gTilesPtr;
static	short			*gTileXlatePtr;

Handle			gPlayfieldHandle = nil;
uint16_t		**gPlayfield = nil;
short			gPlayfieldTileWidth,gPlayfieldTileHeight;
short			gPlayfieldWidth,gPlayfieldHeight;

static	Byte	**gAlternateMap = nil;

static	long	gOldScrollX,gOldScrollY;
long			gScrollX,gScrollY;
long			gTweenedScrollX,gTweenedScrollY;
long			gScrollRow,gScrollCol,gOldScrollRow,gOldScrollCol;

short			gNumItems = -1;
static	ObjectEntryType	**gItemLookupTableX = nil;
ObjectEntryType *gMasterItemList = nil;

TileAttribType	*gTileAttributes;

short			gItemDeleteWindow_Bottom,gItemDeleteWindow_Top,gItemDeleteWindow_Left,gItemDeleteWindow_Right;

static	Rect			gViewWindow,gTargetViewWindow;

static	Boolean			gColorMaskArray[256];							// array of xparent tile colors, false = xparent

static	Boolean			gAltMapFlag = false;

static	long			gShakeyScreenCount = 0;
static	long			gShakeyScreenOffsetX = 0;
static	long			gShakeyScreenOffsetY = 0;

static	Ptr				gMaxItemAddress;								// addr of last item in current item list

// Source port note: moved from TileAnim.c
static	short			gNumTileAnims;
static	TileAnimEntryType	gTileAnims[MAX_TILE_ANIMS];


/**********************/
/*     TABLES         */
/**********************/

#define	MAX_ITEM_NUM	55							// for error checking!

static	Boolean	(*gItemAddPtrs[])(ObjectEntryType *) = {
					AddEnemy_Caveman,
					AddAppearZone,
					NilAdd,						//store
					AddBunny,
					AddEnemy_Triceratops,
					AddEnemy_Turtle,
					AddManEatingPlant,
					AddDinoEgg,
					AddEnemy_BabyDino,
					AddEnemy_Rex,
					NilAdd,						//AddEnemy_ClownBalloon,
					AddEnemy_ClownCar,
					AddJackInTheBox,
					AddEnemy_Clown,
					AddMagicHat,
					AddHealthPOW,
					AddEnemy_FlowerClown,
					AddTeleport,
					AddRaceCar,
					AddKey,
					AddClowndoor,
					AddCandyMPlatform,
					AddCandyDoor,
					AddStar,
					AddEnemy_ChocBunny,
					AddEnemy_GBread,
					AddEnemy_Mint,
					nil,								// was cherrybomb
					AddEnemy_GBear,
					nil,								// my guy init coords
					nil,								// AddFinishLine,
					AddJurassicDoor,
					AddEnemy_Carmel,
					AddWeaponPowerup,
					AddMiscPowerup,
					AddGumBall,
					AddEnemy_LemonDrop,
					AddEnemy_Giant,
					AddEnemy_Dragon,
					AddEnemy_Witch,
					AddEnemy_BBWolf,
					AddEnemy_Soldier,
					AddMuffit,
					AddEnemy_Spider,
					AddFairyDoor,
					AddEnemy_Battery,
					AddPoisonApple,
					AddEnemy_Slinky,
					AddEnemy_8Ball,
					AddShipPOW,
					AddEnemy_Robot,
					AddEnemy_Doggy,
					AddBargainDoor,
					AddEnemy_Top,
					AddHydrant,
					AddKeyColor
				};

void OnChangePlayfieldSize(void)
{
	switch (gGamePrefs.pfSize)
	{
	case PFSIZE_SMALL:
		PF_TILE_WIDTH	= 14;
		PF_TILE_HEIGHT	= 13;				// dimensions of scrolling Playfield
		PF_WINDOW_LEFT	= 24;				// left MUST be on 4 pixel boundary!!!!!
		PF_WINDOW_TOP	= 45;
		break;
	case PFSIZE_MEDIUM:
		PF_TILE_WIDTH	= 21;
		PF_TILE_HEIGHT	= 14;				// dimensions of scrolling Playfield
		PF_WINDOW_LEFT	= 0;				// left MUST be on 4 pixel boundary!!!!!
		PF_WINDOW_TOP	= 0;
		break;
	case PFSIZE_WIDE:
		PF_TILE_WIDTH	= 27;
		PF_TILE_HEIGHT	= 14;				// dimensions of scrolling Playfield
		PF_WINDOW_LEFT	= 0;				// left MUST be on 4 pixel boundary!!!!!
		PF_WINDOW_TOP	= 0;
		break;
	default:
		GAME_ASSERT_MESSAGE(false, "OnChangePlayfieldSize: Unsupported pfSize!");
	}

	if (gGamePrefs.pfSize != PFSIZE_SMALL)
	{
		VISIBLE_WIDTH = ((int32_t)PF_TILE_WIDTH - 1) * TILE_SIZE;
		VISIBLE_HEIGHT = ((int32_t)PF_TILE_HEIGHT + 1) * TILE_SIZE;
	}
	else
	{
		VISIBLE_WIDTH = 640;
		VISIBLE_HEIGHT = 480;
	}

	GAME_ASSERT(VISIBLE_WIDTH >= 640);
	GAME_ASSERT(VISIBLE_HEIGHT >= 480);

	SetOptimalWindowSize();

	MakeGameWindow();

	OnChangeIntegerScaling();
}

/****************** CLEAR TILE COLOR MASKS ***************/
//
// Initializes the array of flags which indicate whether a
// color is xparent or not with prioritized sprites.
//

void ClearTileColorMasks(void)
{
long		i;

	for (i=0; i < 256; i++)
	{
		gColorMaskArray[i] = true;				// assume nothing is xparent
	}
}


/********************* LOAD TILESET **********************/
//
// Loads a tileset into memory & does all initialization.
//


void LoadTileSet(const char* fileName)
{
Ptr	tileSetPtr					= nil;
Ptr tileAnimList				= nil;
int16_t* tileXparentList		= nil;

	ClearTileColorMasks();									// clear this to begin with

	if (gTileSetHandle != nil)								// see if zap old tileset
		DisposeHandle(gTileSetHandle);

	gTileSetHandle = LoadPackedFile(fileName);				// load the file
	tileSetPtr = *gTileSetHandle;							// get fixed ptr

			/* GET OFFSETS */

	int offsetToTileDefinitions			= UnpackI32BEInPlace(tileSetPtr+6)+2;		// base + offset + 2 (skip # tiles word)
	int offsetToXlateTable				= UnpackI32BEInPlace(tileSetPtr+10)+2;	// base + offset + 2 (skip # entries word)
	int offsetToTileAttributes			= UnpackI32BEInPlace(tileSetPtr+14)+2;	// base + offset + 2 (skip # entries word)
	int offsetToTileAnimList			= UnpackI32BEInPlace(tileSetPtr+22)+2;
	int offsetToTileXparentColorList	= UnpackI32BEInPlace(tileSetPtr+26)+2;

	GAME_ASSERT(offsetToTileDefinitions	< offsetToXlateTable);
	GAME_ASSERT(offsetToXlateTable		< offsetToTileAttributes);
	GAME_ASSERT(offsetToTileAttributes	< offsetToTileAnimList);
	GAME_ASSERT(offsetToTileAnimList	< offsetToTileXparentColorList);

			/* GET ENTRY COUNTS */

	/*int numTileDefinitions		=*/   UnpackI16BEInPlace(tileSetPtr + offsetToTileDefinitions			- 2	);
	int numXlateEntries					= UnpackI16BEInPlace(tileSetPtr + offsetToXlateTable				- 2	);
	int numTileAttributeEntries			= UnpackI16BEInPlace(tileSetPtr + offsetToTileAttributes			- 2	);
	gNumTileAnims						= UnpackI16BEInPlace(tileSetPtr + offsetToTileAnimList			- 2	);
	int numTileXparentColors			= UnpackI16BEInPlace(tileSetPtr + offsetToTileXparentColorList	- 2	);

			/* GET POINTERS TO TABLES */

	gTilesPtr			=						(	tileSetPtr + offsetToTileDefinitions		);
	gTileXlatePtr		=	(int16_t *)			(	tileSetPtr + offsetToXlateTable				);
	gTileAttributes		=	(TileAttribType *)	(	tileSetPtr + offsetToTileAttributes			);
	tileAnimList		=						(	tileSetPtr + offsetToTileAnimList			);
	tileXparentList		=	(int16_t *)			(	tileSetPtr + offsetToTileXparentColorList	);

			/* BYTESWAP STUFF */

	// Byteswap gTileXlatePtr
	UnpackIntsBE(2, numXlateEntries, gTileXlatePtr);

	// Byteswap gTileAttributes
	UnpackStructs(">Hh4b", sizeof(TileAttribType), numTileAttributeEntries, gTileAttributes);

	// Byteswap tileXparentList
	UnpackIntsBE(2, numTileXparentColors, tileXparentList);

	/***************** PREPARE TILE ANIMS ***********************/
	//
	// Source port note: moved from TileAnim.c
	//

	GAME_ASSERT(gNumTileAnims >= 0);
	GAME_ASSERT(gNumTileAnims <= MAX_TILE_ANIMS);
	Ptr currentTileAnimData = tileAnimList;
	for (int i = 0; i < gNumTileAnims; i++)
	{
		GAME_ASSERT(HandleBoundsCheck(gTileSetHandle, currentTileAnimData));

		// Get name (fixed-size pascal string, max length: 1 length byte + 15 chars)
#if _DEBUG
		char name[16];
		uint8_t nameLength = *currentTileAnimData;
		if (nameLength > 15)	// the name is always truncated to max 15 characters, whatever the file says
			nameLength = 15;
		BlockMove(currentTileAnimData+1, name, nameLength);
		name[nameLength] = '\0';
#endif

		TileAnimDefType* tileAnimDef = (TileAnimDefType*) (currentTileAnimData + 16);
		UnpackIntsBE(2, 3, tileAnimDef);										// byteswap speed, baseTile, numFrames
		UnpackIntsBE(2, tileAnimDef->numFrames, tileAnimDef->tileNums);			// byteswap tileNums array

#if _DEBUG
//		printf("PrepareTileAnims #%d: \"%s\", %d frames\n", i, name, tileAnimDef->numFrames);
#endif

		// Set tile anim
		gTileAnims[i].count = 0;
		gTileAnims[i].index = 0;
		gTileAnims[i].defPtr = tileAnimDef;

		// Advance pointer to next tile anim data
		currentTileAnimData += 16 + 2*3 + 2*tileAnimDef->numFrames;
	}


	/******************** SET TILE COLOR MASKS *********************/
	//
	// Activates color xparency on certain colors in palette.
	//

	for (int i = 0; i < numTileXparentColors; i++)
	{
		GAME_ASSERT(HandleBoundsCheck(gTileSetHandle, (Ptr) &tileXparentList[i]));
		GAME_ASSERT(tileXparentList[i] >= 0);
		GAME_ASSERT(tileXparentList[i] < (int)sizeof(gColorMaskArray));

		gColorMaskArray[tileXparentList[i]] = false;
	}
}


/******************** DISPOSE CURRENT MAP DATA *********************/
//
// Disposes of all playfield related data ptrs and handles
//

void DisposeCurrentMapData(void)
{
	if (gPlayfieldHandle != nil)					// see if zap old playfield
	{
		DisposeHandle(gPlayfieldHandle);
		gPlayfieldHandle = nil;
	}

	if (gPlayfield != nil)
	{
		DisposePtr((Ptr)gPlayfield);
		gPlayfield = nil;
	}

	if (gAlternateMap != nil)
	{
		DisposePtr((Ptr)gAlternateMap);
		gAlternateMap = nil;
	}


	if (gTileSetHandle != nil)						// see if zap old tileset
	{
		DisposeHandle(gTileSetHandle);
		gTileSetHandle = nil;
	}

	gNumItems = -1;
	gMasterItemList = nil;	// this is just a pointer within gPlayfieldHandle, no need to dispose of it

}


/************************ LOAD PLAYFIELD *************************/
//
// NOTE: Assumes that previous playfield data has already been deleted
//

void LoadPlayfield(const char* fileName)
{
uint16_t	*tempPtr;
long	i;
Ptr		bytePtr,pfPtr;

	gPlayfieldHandle = LoadPackedFile(fileName);					// load the file

	pfPtr = *gPlayfieldHandle;										// get fixed ptr


	int32_t offsetToMapImage		= UnpackI32BEInPlace(pfPtr + 2);
	int32_t offsetToAltMap			= UnpackI32BEInPlace(pfPtr + 10);

				/* BUILD MAP ARRAY */

	tempPtr = (uint16_t *)(pfPtr + offsetToMapImage);				// point to MAP_IMAGE
	UnpackIntsBE(2, 2, tempPtr);									// byteswap width/height
	gPlayfieldTileWidth = *(tempPtr++);								// get dimensions
	gPlayfieldTileHeight = *(tempPtr++);
	gPlayfieldWidth = gPlayfieldTileWidth<<TILE_SIZE_SH;
	gPlayfieldHeight = gPlayfieldTileHeight<<TILE_SIZE_SH;

	gPlayfield = (uint16_t **)NewPtrClear(sizeof(uint16_t *) * gPlayfieldTileHeight);	// alloc memory for 1st dimension of matrix
	GAME_ASSERT(gPlayfield);
	for (i = 0; i < gPlayfieldTileHeight; i++)						// build 1st dimension of matrix
	{
		UnpackIntsBE(2, gPlayfieldTileWidth, tempPtr);				// byteswap row
		gPlayfield[i]= (unsigned short *)tempPtr;					// set pointer to row
		tempPtr += gPlayfieldTileWidth;								// next row
	}


			/* GET ALTERNATE MAP */

	bytePtr = (Ptr)(pfPtr + offsetToAltMap);						// point to ALTERNATE_MAP
	if (bytePtr == nil)
	{
		gAlternateMap = nil;										// no alt map to load
		gAltMapFlag = false;
	}
	else
	{
		gAlternateMap = (Byte **)NewPtrClear(sizeof(Byte *) * gPlayfieldTileHeight);	// alloc memory for 1st dimension of matrix
		GAME_ASSERT(gAlternateMap);
		for (i = 0; i < gPlayfieldTileHeight; i++)					// build matrix
		{
			gAlternateMap[i]= (Byte *)bytePtr;						// set pointer to row
			bytePtr += gPlayfieldTileWidth;							// next row
		}
		gAltMapFlag = true;
	}

	gScrollX = 0;													// default these
	gScrollY = 0;
	gOldScrollX = 0;
	gOldScrollY = 0;
	gTweenedScrollX = 0;
	gTweenedScrollY = 0;
}


/*************** INIT PLAYFIELD *******************/
//
// Draws entire playfield @ current scroll coords
// into playfield buffer
//

void InitPlayfield(void)
{
long		row,col,x,y,col2;
long		right,left,top,bottom;

				/* INIT PLAYFIELD CLIPPING REGION */

	gRegionClipTop[CLIP_REGION_PLAYFIELD] = OFFSCREEN_WINDOW_TOP+PF_WINDOW_TOP;
	gRegionClipBottom[CLIP_REGION_PLAYFIELD] = OFFSCREEN_WINDOW_TOP+PF_WINDOW_TOP+PF_WINDOW_HEIGHT;
	gRegionClipLeft[CLIP_REGION_PLAYFIELD] = OFFSCREEN_WINDOW_LEFT+PF_WINDOW_LEFT;
	gRegionClipRight[CLIP_REGION_PLAYFIELD] = OFFSCREEN_WINDOW_LEFT+PF_WINDOW_LEFT+PF_WINDOW_WIDTH+70;


					/* INIT VIEW WINDOW */

	gViewWindow.left = gTargetViewWindow.left = SCROLL_WINDOW_LEFT;
	gViewWindow.right = gTargetViewWindow.right = SCROLL_WINDOW_RIGHT;
	gViewWindow.top = gTargetViewWindow.top = SCROLL_WINDOW_TOP;
	gViewWindow.bottom = gTargetViewWindow.bottom = SCROLL_WINDOW_BOTTOM;

	gShakeyScreenCount = 0;										// init shakey screen

				/* VERIFY INITIAL SCROLL POSITION */

	gScrollX = gMyX-(PF_WINDOW_WIDTH/2);						// attempt to center me on screen
	gScrollY = gMyY-(PF_WINDOW_HEIGHT/2);

	if (gScrollX < SCROLL_BORDER)
		gScrollX = SCROLL_BORDER;
	else
	if (gScrollX > ((gPlayfieldWidth)-(SCROLL_BORDER+PF_WINDOW_WIDTH)))
		gScrollX = (gPlayfieldWidth)-(SCROLL_BORDER+PF_WINDOW_WIDTH);

	if (gScrollY < SCROLL_BORDER)
		gScrollY = SCROLL_BORDER;
	else
	if (gScrollY > ((gPlayfieldHeight)-(SCROLL_BORDER+PF_WINDOW_HEIGHT)))
		gScrollY = (gPlayfieldHeight)-(SCROLL_BORDER+PF_WINDOW_HEIGHT);

	gOldScrollX = gScrollX;
	gOldScrollY = gScrollY;
	gTweenedScrollX = gScrollX;
	gTweenedScrollY = gScrollY;

//				/* BUILD ITEM LIST */
//
//	BuildItemList();

				/* DRAW INITIAL PLAYFIELD */

	gScrollRow = gOldScrollRow = gScrollY>>TILE_SIZE_SH;		// calc scroll tile row/col
	gScrollCol = gOldScrollCol = gScrollX>>TILE_SIZE_SH;

	row = gScrollRow % PF_TILE_HEIGHT;							// calc row in buffer
	col = gScrollCol % PF_TILE_WIDTH;							// calc col in buffer

	for (y = 0; y < PF_TILE_HEIGHT; y++)
	{
		col2 = col;
		for (x = 0; x < PF_TILE_WIDTH; x++)
		{
			DrawATile(gPlayfield[gScrollRow+y][gScrollCol+x],row,col2,true);

			if (++col2 >= PF_TILE_WIDTH)
				col2 = 0;
		}
		if (++row >= PF_TILE_HEIGHT)
			row = 0;
	}

				/* ADD ITEMS IN THIS AREA */

	SetItemDeleteWindow();

	right = gScrollCol+PF_TILE_WIDTH+ITEM_WINDOW_RIGHT;				// create area
	if (right >= gPlayfieldTileWidth)								// check bounds
		right = gPlayfieldTileWidth-1;
	left = gScrollCol-ITEM_WINDOW_LEFT;
	if (left < 0)
		left = 0;
	top = gScrollRow-ITEM_WINDOW_TOP;
	if (top < 0)
		top = 0;
	bottom = gScrollRow+PF_TILE_HEIGHT+ITEM_WINDOW_BOTTOM;
	if (bottom >= gPlayfieldTileHeight)
		bottom = gPlayfieldTileHeight-1;

	ScanForPlayfieldItems(top,bottom,left,right);				// scan for any items

}


/************************ BUILD ITEM LIST ***********************/
//
// Build sorted lists of playfield items
//

void BuildItemList(void)
{
long	offset;
long	col,itemCol,itemNum,nextCol,prevCol;
ObjectEntryType *lastPtr;

	if	(gPlayfieldTileWidth > MAX_PLAYFIELD_WIDTH)				// see if bigger than max allowed
		DoFatalAlert("gPlayfieldTileWidth is greater than MAX_PLAYFIELD_WIDTH!");

					/* GET BASIC INFO */

	offset = UnpackI32BE(*gPlayfieldHandle + 6);				// get offset to OBJECT_LIST
	gNumItems = UnpackI16BE(*gPlayfieldHandle + offset);		// get # items in file
	if (gNumItems == 0)
		return;
	gMasterItemList = (ObjectEntryType *)(*gPlayfieldHandle+offset+2);	// point to items in file

					/* BYTESWAP ALL OBJECT ENTRY STRUCTS */

	// Ensure the in-memory representation of the struct is tightly-packed to match the struct's layout on disk
	_Static_assert(sizeof(struct ObjectEntryType) == 4+4+2+4, "ObjectEntryType has incorrect size!");

	UnpackStructs(">2ih4b", sizeof(ObjectEntryType), gNumItems, gMasterItemList);

				/* BUILD HORIZ LOOKUP TABLE */

	gMaxItemAddress = (Ptr)&gMasterItemList[gNumItems-1];		// remember addr of last item
	lastPtr = &gMasterItemList[0];
	nextCol = 0;												// start @ col 0
	prevCol = -1;
	for (itemNum = 0; itemNum < gNumItems; itemNum++)
	{
		itemCol = gMasterItemList[itemNum].x>>TILE_SIZE_SH;		// get column of item
		if (itemCol != prevCol)									// see if changed
		{
			for (col = nextCol; col <= itemCol; col++)			// filler pointers
				gItemLookupTableX[col] = &gMasterItemList[itemNum];
			prevCol = itemCol;
			nextCol = itemCol+1;
			lastPtr = &gMasterItemList[itemNum];
		}
	}
	for (col = nextCol; col < gPlayfieldTileWidth; col++)		// set trailing column pointers
		gItemLookupTableX[col] = lastPtr;

}


/************************** SCROLL PLAYFIELD ****************************/
//
// Scrolls playfield to current gScrollX/Y coords.
//
// NOTE: max scroll speed is TILE_SIZE pixels
//

void ScrollPlayfield(void)
{

	gOldScrollRow = gScrollRow;									// save old row/col
	gOldScrollCol = gScrollCol;

	if (gTweenFrameFactor.L <= 0)
	{
		gTweenedScrollX = gOldScrollX;
		gTweenedScrollY = gOldScrollY;
	}
	else if (gTweenFrameFactor.L >= 0x10000)
	{
		gTweenedScrollX = gScrollX;
		gTweenedScrollY = gScrollY;
	}
	else if (gTweenFrameFactor.L < 0x10000)						// interpolate camera position
	{
		int32_t dx = (int32_t)(gOldScrollX - gScrollX);
		int32_t dy = (int32_t)(gOldScrollY - gScrollY);
		gTweenedScrollX = gScrollX + Fix32_Int(Fix32_Mul(dx << 16, gOneMinusTweenFrameFactor.L));
		gTweenedScrollY = gScrollY + Fix32_Int(Fix32_Mul(dy << 16, gOneMinusTweenFrameFactor.L));
	}
	
	gScrollCol = gTweenedScrollX / TILE_SIZE;			// get new row/col
	gScrollRow = gTweenedScrollY / TILE_SIZE;


			/* SEE IF SCROLLED A TILE VERTICALLY */

	if (gOldScrollRow != gScrollRow)
	{
		if (gScrollRow > gOldScrollRow)							// see which way
			ScrollPlayfield_Down();
		else
			ScrollPlayfield_Up();
	}

			/* SEE IF SCROLLED A TILE HORIZONTALLY */

	if (gOldScrollCol != gScrollCol)
	{
		if (gScrollCol > gOldScrollCol)							// see which way
			ScrollPlayfield_Right();
		else
			ScrollPlayfield_Left();
	}

			/* CALC ITEM OUTER BOUNDARY WINDOW */

	SetItemDeleteWindow();
}

/************************** STOP SCROLLING PLAYFIELD ****************************/
//
// Call this when the camera goes static.
// This will stop movement interpolation on the camera and prevent the playfield from jittering.
//

void StopScrollingPlayfield(void)
{
	gOldScrollX = gScrollX;
	gOldScrollY = gScrollY;
}

/****************** SET ITEM DELETE WINDOW ****************/
//
// Sets coords of the window/zone out of which items should delete
//

void SetItemDeleteWindow(void)
{
	gItemDeleteWindow_Top = (gScrollRow-ITEM_WINDOW_TOP-OUTER_SIZE)<<TILE_SIZE_SH;
	gItemDeleteWindow_Bottom = (gScrollRow+PF_TILE_HEIGHT+ITEM_WINDOW_BOTTOM+OUTER_SIZE)<<TILE_SIZE_SH;
	gItemDeleteWindow_Left = (gScrollCol-ITEM_WINDOW_LEFT-OUTER_SIZE)<<TILE_SIZE_SH;
	gItemDeleteWindow_Right = (gScrollCol+PF_TILE_WIDTH+ITEM_WINDOW_RIGHT+OUTER_SIZE)<<TILE_SIZE_SH;
}

/****************** SCROLL PLAYFIELD: DOWN **********************/

void ScrollPlayfield_Down(void)
{
long	row,col,x,mapRow,right;

				/* UPDATE TILES */

	mapRow = gScrollRow+(PF_TILE_HEIGHT-1);						// calc row in map matrix

	row = mapRow % PF_TILE_HEIGHT;								// calc row in buffer
	col = gScrollCol % PF_TILE_WIDTH;							// calc col in buffer

	for (x = 0; x < PF_TILE_WIDTH; x++)
	{
		DrawATile(gPlayfield[mapRow][gScrollCol+x],row,col,true);

		if (++col >= PF_TILE_WIDTH)
			col = 0;
	}

				/* UPDATE ITEMS */

	x = gScrollCol-ITEM_WINDOW_LEFT;							// calc area to check for new items
	if (x < 0)													// see if out of bounds
		x = 0;
	right = gScrollCol+PF_TILE_WIDTH+ITEM_WINDOW_RIGHT;
	if (right >= gPlayfieldTileWidth)
		right = gPlayfieldTileWidth-1;
	mapRow = gScrollRow+PF_TILE_HEIGHT+ITEM_WINDOW_BOTTOM;
	if (mapRow >= gPlayfieldTileHeight)
		mapRow = gPlayfieldTileHeight-1;

	ScanForPlayfieldItems(mapRow,mapRow,x,right);				// scan for any items
}


/****************** SCROLL PLAYFIELD: UP **********************/

void ScrollPlayfield_Up(void)
{
long	row,col,x,right;

				/* UPDATE TILES */

	row = gScrollRow % PF_TILE_HEIGHT;								// calc row in buffer
	col = gScrollCol % PF_TILE_WIDTH;								// calc col in buffer

	for (x = 0; x < PF_TILE_WIDTH; x++)
	{
		DrawATile(gPlayfield[gScrollRow][gScrollCol+x],row,col,true);
		if (++col >= PF_TILE_WIDTH)
			col = 0;
	}


				/* UPDATE ITEMS */

	x = gScrollCol-ITEM_WINDOW_LEFT;							// calc area to check for new items
	if (x < 0)													// see if out of bounds
		x = 0;
	right = gScrollCol+PF_TILE_WIDTH+ITEM_WINDOW_RIGHT;
	if (right >= gPlayfieldTileWidth)
		right = gPlayfieldTileWidth-1;
	row = gScrollRow-ITEM_WINDOW_TOP;
	if (row < 0)
		row = 0;

	ScanForPlayfieldItems(row,row,x,right);						// scan for any items

}


/****************** SCROLL PLAYFIELD: RIGHT **********************/
//
// playfield actually scrolls to the left, but new data added on right side
//

void ScrollPlayfield_Right(void)
{
long	row,col,y,mapCol,mapRowTop,mapRowBot;

				/* UPDATE TILES */

	mapCol = gScrollCol+(PF_TILE_WIDTH-1);						// calc col in map matrix
	row = gScrollRow % PF_TILE_HEIGHT;							// calc row in buffer
	col = mapCol  % PF_TILE_WIDTH;								// calc col in buffer

	for (y = 0; y < PF_TILE_HEIGHT; y++)
	{
		DrawATile(gPlayfield[gScrollRow+y][mapCol],row,col,true);
		if (++row >= PF_TILE_HEIGHT)
			row = 0;
	}

				/* UPDATE ITEMS */

	mapCol = gScrollCol+PF_TILE_WIDTH+ITEM_WINDOW_RIGHT;		// calc column to check for new objects
	if (mapCol >= gPlayfieldTileWidth)								// see if out of bounds
		return;

	mapRowTop = gScrollRow-ITEM_WINDOW_TOP;						// calc top/bottom bounds
	if (mapRowTop < 0)
		mapRowTop = 0;
	mapRowBot = gScrollRow+PF_TILE_HEIGHT+ITEM_WINDOW_BOTTOM;
	if (mapRowBot >= gPlayfieldTileHeight)
		mapRowBot = gPlayfieldTileHeight-1;

	ScanForPlayfieldItems(mapRowTop,mapRowBot,mapCol,mapCol);	// scan for any items

}

/****************** SCROLL PLAYFIELD: LEFT **********************/

void ScrollPlayfield_Left(void)
{
long	row,col,y,mapRowTop,mapRowBot,mapCol;

				/* UPDATE TILES */

	row = gScrollRow % PF_TILE_HEIGHT;							// calc row in buffer
	col = gScrollCol  % PF_TILE_WIDTH;								// calc col in buffer

	for (y = 0; y < PF_TILE_HEIGHT; y++)
	{
		DrawATile(gPlayfield[gScrollRow+y][gScrollCol],row,col,true);
		if (++row >= PF_TILE_HEIGHT)
			row = 0;
	}

				/* UPDATE ITEMS */

	mapCol = gScrollCol-ITEM_WINDOW_RIGHT;						// calc column to check for new objects
	if (mapCol < 0)												// see if out of bounds
		return;

	mapRowTop = gScrollRow-ITEM_WINDOW_TOP;						// calc top/bottom bounds
	if (mapRowTop < 0)
		mapRowTop = 0;
	mapRowBot = gScrollRow+PF_TILE_HEIGHT+ITEM_WINDOW_BOTTOM;
	if (mapRowBot >= gPlayfieldTileHeight)
		mapRowBot = gPlayfieldTileHeight-1;


	ScanForPlayfieldItems(mapRowTop,mapRowBot,mapCol,mapCol);	// scan for any items
}


/****************** SCAN FOR PLAYFIELD ITEMS *******************/
//
// Given this range, scan for items.  Coords are in row/col values.
//

void ScanForPlayfieldItems(long top, long bottom, long left, long right)
{
ObjectEntryType *itemPtr;
long	row,type;
Boolean		flag;

	if (gNumItems == 0)
		return;

	itemPtr = gItemLookupTableX[left];								// get pointer to 1st item at this X

	while (((itemPtr->x>>TILE_SIZE_SH) >= left) && ((itemPtr->x>>TILE_SIZE_SH) <= right))	// check all items in this column range
	{
		row = itemPtr->y>>TILE_SIZE_SH;
		if ((row >= top) && (row <= bottom))						// & this row range
		{
					/* ADD AN ITEM */

			if (!(itemPtr->type&ITEM_IN_USE))						// see if item available
			{
				type = itemPtr->type&ITEM_NUM;						// mask out status bits 15..12
				if (type > MAX_ITEM_NUM)							// error check!
					DoFatalAlert("Illegal Map Item Type!");
				else
				{
					flag = gItemAddPtrs[type](itemPtr);				// call item's ADD routine
					if (flag)
						itemPtr->type |= ITEM_IN_USE;				// set in-use flag
				}
			}
		}
		itemPtr++;													// point to next item
		if ((Ptr) itemPtr > gMaxItemAddress)						// see if its past the last address
			break;
	}
}


/************************ DO MY SCREEN SCROLL ************************/
//
// Update screen scroll based on MyGuy's position
//

void DoMyScreenScroll(void)
{
long	screenX,screenY;
long	scrollDX, scrollDY;

	gOldScrollX = gScrollX;
	gOldScrollY = gScrollY;

	UpdateViewWindow();

	scrollDX = scrollDY = 0;								// assume no scroll

	if (!gScreenScrollFlag)
		return;

	screenX = gMyNodePtr->X.Int-gScrollX;					// calc screen coords
	screenY = gMyNodePtr->Y.Int-gScrollY;

	if (screenX < gViewWindow.left)							// see if scroll to left
	{
		scrollDX = -(gViewWindow.left-screenX);
	}
	else
	if (screenX > gViewWindow.right)						// see if scroll to right
	{
		scrollDX = screenX-gViewWindow.right;
	}

	if (screenY < gViewWindow.top)							// see if scroll up
	{
		scrollDY = -(gViewWindow.top-screenY);
	}
	else
	if (screenY > gViewWindow.bottom)						// see if scroll down
	{
		scrollDY = screenY-gViewWindow.bottom;
	}


	if (scrollDX > (TILE_SIZE-1))								// limit scroll speed
		scrollDX = (TILE_SIZE-1);
	else
	if (scrollDX < -(TILE_SIZE-1))
		scrollDX = -(TILE_SIZE-1);

	if (scrollDY > (TILE_SIZE-1))
		scrollDY = (TILE_SIZE-1);
	else
	if (scrollDY < -(TILE_SIZE-1))
		scrollDY = -(TILE_SIZE-1);


	gScrollX += scrollDX;									// move it
	gScrollY += scrollDY;

	if (gScrollX < SCROLL_BORDER)							// check edges
	{
		gScrollX = SCROLL_BORDER;
		scrollDX = 0;
	}
	else
	if (gScrollX > ((gPlayfieldWidth)-(SCROLL_BORDER+PF_WINDOW_WIDTH)))
	{
		gScrollX = (gPlayfieldWidth)-(SCROLL_BORDER+PF_WINDOW_WIDTH);
		scrollDX = 0;
	}

	if (gScrollY < SCROLL_BORDER)
	{
		gScrollY = SCROLL_BORDER;
		scrollDY = 0;
	}
	else
	if (gScrollY > ((gPlayfieldHeight)-(SCROLL_BORDER+PF_WINDOW_HEIGHT)))
	{
		gScrollY = (gPlayfieldHeight)-(SCROLL_BORDER+PF_WINDOW_HEIGHT);
		scrollDY = 0;
	}

	if (!(scrollDX || scrollDY))							// stop teleporting when screen stops
		gTeleportingFlag = false;
}


/******************** UPDATE VIEW WINDOW ***********************/
//
// Moves the view window depending on MyGuy's direction
//

void UpdateViewWindow(void)
{
long	dir;

	dir = gMyDirection;

	if (!(gMyDX | gMyDY))											// if Im not moving, but Im "moving" then rethink how to aim me
	{
		if (gMySumDX | gMySumDY)
		{
			if (gMySumDY < 0)										// see if going up
				dir = AIM_UP;
			else
			if (gMySumDY > 0)										// see if going down
				dir = AIM_DOWN;
			else
			if (gMySumDX < 0)										// see if going left
				dir = AIM_LEFT;
			else
			if (gMySumDX > 0)										// see if going right
				dir = AIM_RIGHT;
		}
	}

				/* CHECK HORIZ SIDES */

	if ((dir == AIM_UP) || (dir == AIM_DOWN))						// if vertical, then center horiz
	{
		gTargetViewWindow.left = SCROLL_WINDOW_LEFT;
		gTargetViewWindow.right = SCROLL_WINDOW_RIGHT;
	}
	else
	if ((dir > AIM_UP) && (dir < AIM_DOWN))							// see if aiming right
	{
		gTargetViewWindow.left = SCROLL_WINDOW_LEFT-VIEW_FACTOR;
		gTargetViewWindow.right = SCROLL_WINDOW_RIGHT-VIEW_FACTOR;
	}
	else																// otherwise, aiming left
	{
		gTargetViewWindow.left = SCROLL_WINDOW_LEFT+VIEW_FACTOR;
		gTargetViewWindow.right = SCROLL_WINDOW_RIGHT+VIEW_FACTOR;
	}


				/* CHECK VERTICAL SIDES */

	if ((dir == AIM_RIGHT) || (dir == AIM_LEFT))						// if horiz, then center vert
	{
		gTargetViewWindow.top = SCROLL_WINDOW_TOP;
		gTargetViewWindow.bottom = SCROLL_WINDOW_BOTTOM;
	}
	else
	if ((dir > AIM_RIGHT) && (dir < AIM_LEFT))							// see if aiming down
	{
		gTargetViewWindow.top = SCROLL_WINDOW_TOP-VIEW_FACTOR;
		gTargetViewWindow.bottom = SCROLL_WINDOW_BOTTOM-VIEW_FACTOR;
	}
	else																// otherwise, aiming up
	{
		gTargetViewWindow.top = SCROLL_WINDOW_TOP+VIEW_FACTOR;
		gTargetViewWindow.bottom = SCROLL_WINDOW_BOTTOM+VIEW_FACTOR;
	}


				/* MOVE VIEW TO TARGET VIEW */

	if (gViewWindow.top < gTargetViewWindow.top)
		gViewWindow.top += 4;
	else
	if (gViewWindow.top > gTargetViewWindow.top)
		gViewWindow.top -= 4;

	if (gViewWindow.bottom < gTargetViewWindow.bottom)
		gViewWindow.bottom += 4;
	else
	if (gViewWindow.bottom > gTargetViewWindow.bottom)
		gViewWindow.bottom -= 4;

	if (gViewWindow.left < gTargetViewWindow.left)
		gViewWindow.left += 4;
	else
	if (gViewWindow.left > gTargetViewWindow.left)
		gViewWindow.left -= 4;

	if (gViewWindow.right < gTargetViewWindow.right)
		gViewWindow.right += 4;
	else
	if (gViewWindow.right > gTargetViewWindow.right)
		gViewWindow.right -= 4;

}




/*********************** TEST COORDINATE RANGE ************************/
//
// Returns true if global X/Y coords are out of playfield item window
//

Boolean TestCoordinateRange(void)
{

	if ((gX.Int < gItemDeleteWindow_Left) || (gX.Int > gItemDeleteWindow_Right) ||
		(gY.Int < gItemDeleteWindow_Top) || (gY.Int > gItemDeleteWindow_Bottom))
		return(true);
	else
		return(false);
}


/*********************** TRACK ITEM ************************/
//
// Returns true if gThisNodePtr is out if range
//

Boolean TrackItem(void)
{

	if ((gThisNodePtr->X.Int < gItemDeleteWindow_Left) ||
		(gThisNodePtr->X.Int > gItemDeleteWindow_Right) ||
		(gThisNodePtr->Y.Int < gItemDeleteWindow_Top) ||
		(gThisNodePtr->Y.Int > gItemDeleteWindow_Bottom))
	{
		return(true);
	}
	else
		return(false);
}


/********************* GET ALTERNATE TILE INFO **********************/
//
// Returns the alternate tile # at the given coordinates
//

Byte GetAlternateTileInfo(uint16_t x, uint16_t y)
{
	if ((y >= gPlayfieldHeight) || (x >= gPlayfieldWidth))		// check for bounds error (automatically checks for <0)
		return 0;

	return(gAlternateMap[y>>TILE_SIZE_SH][x>>TILE_SIZE_SH]);
}


/********************* GET MAP TILE ATTRIBS **********************/
//
// Returns the attrib bits of the tile @ x,y
//

uint16_t GetMapTileAttribs(uint16_t x, uint16_t y)
{
	if ((y >= gPlayfieldHeight) || (x >= gPlayfieldWidth))		// check for bounds error  (automatically checks for <0)
		return(0);

	return gTileAttributes[gPlayfield[y>>TILE_SIZE_SH][x>>TILE_SIZE_SH]&TILENUM_MASK].bits;
}


/********************* GET FULL MAP TILE ATTRIBS **********************/
//
// Returns a pointer to ALL the attribs of the tile @ x,y
//
//

const TileAttribType *GetFullMapTileAttribs(uint16_t x, uint16_t y)
{
	if ((y >= gPlayfieldHeight) || (x >= gPlayfieldWidth))		// check for bounds error  (automatically checks for <0)
		return(nil);

	return (&(gTileAttributes[gPlayfield[y>>TILE_SIZE_SH][x>>TILE_SIZE_SH]&TILENUM_MASK]));
}



/****************** START SHAKEY SCREEN ***********************/
//
// Gets the screen shaking for "duration" ticks
//

void StartShakeyScreen(short duration)
{
	if (gShakeyScreenCount < duration)			// only reset if new duration is longer than current
		gShakeyScreenCount = duration;
}


void UpdateShakeyScreen(void)
{
	if (gShakeyScreenCount)
	{
		gShakeyScreenOffsetX = (MyRandomLong() & 0b111ul) - 4;
		gShakeyScreenOffsetY = (MyRandomLong() & 0b111ul) - 4;
		gShakeyScreenCount--;
	}
	else
	{
		gShakeyScreenOffsetX = 0;
		gShakeyScreenOffsetY = 0;
	}
}


/******************** MOVE ON PATH *******************/
//
// Moves given object along the Alternate Map's path tiles
//
// INPUT: gTheNodePtr = ptr to current node
//		gDX/Y etc. all set
//
// OUTPUT: tile that we're on
//

short MoveOnPath(long	speed, Boolean useDefault)
{
Byte	tile;

	switch(tile = GetAlternateTileInfo(gX.Int, gY.Int))			// see which way to go
	{
		case	ALT_TILE_DIR_UP:
				gDY = -speed;
				gDX = 0;
				break;

		case	ALT_TILE_DIR_RIGHT:
				gDX = speed;
				gDY = 0;
				break;

		case	ALT_TILE_DIR_DOWN:
				gDY = speed;
				gDX = 0;
				break;

		case	ALT_TILE_DIR_LEFT:
				gDX = -speed;
				gDY = 0;
				break;

		default:
				if (useDefault)			// if default flag, then stop
				{
					gDX = 0;
					gDY = 0;
				}
	}

	gX.L += gDX;
	gY.L += gDY;

	return(tile);
}


/******************** NIL ADD ***********************/
//
// nothing add
//

Boolean NilAdd(ObjectEntryType *itemPtr)
{
	(void)itemPtr;	// unused
	return(false);
}


/******************* CREATE PLAYFIELD PERMANENT MEMORY ***********************/
//
// Create playfield memory items that ARE NEVER PURGED.. EVER!
//

void CreatePlayfieldPermanentMemory(void)
{
Handle		tempHand;

				/* ALLOC MEMORY FOR ITEM LOOKUP TABLE */

	tempHand = NewHandleClear(sizeof(ObjectEntryType *)*MAX_PLAYFIELD_WIDTH);
	GAME_ASSERT(tempHand);
	gItemLookupTableX = (ObjectEntryType **)*tempHand;
}


/************************ DRAW A TILE ***********************/

void DrawATile(unsigned short tileNum, short row, short col, Boolean maskFlag)
{
unsigned char *destPtr,*srcPtr,*destCopyPtr;
long		height,i;
Ptr			destStartPtr,destCopyStartPtr;
unsigned char	*copyOfSrc;
unsigned long	rowS,colS;								// shifted version of row & col
Byte		pixel;
Ptr			destPtrB,srcPtrB;

					/* CALC DEST POINTERS */

	destStartPtr = (Ptr)(gPFLookUpTable[rowS = row<<TILE_SIZE_SH]+(colS = col<<TILE_SIZE_SH));
	destCopyStartPtr = (Ptr)(gPFCopyLookUpTable[rowS]+colS);

					/* CALC TILE DEFINITION ADDR */

	GAME_ASSERT(HandleBoundsCheck(gTileSetHandle, (Ptr) &gTileXlatePtr[tileNum & TILENUM_MASK]));

	int xlate = gTileXlatePtr[tileNum&TILENUM_MASK];

	copyOfSrc = srcPtr = (unsigned char *)(gTilesPtr + (xlate<<(TILE_SIZE_SH*2)));
	destPtr = (unsigned char *)destStartPtr;
	destCopyPtr = (unsigned char *)destCopyStartPtr;

						/* DRAW THE TILE */

	height = TILE_SIZE;
	do
	{
		for (i = 0; i < TILE_SIZE; i++)
			destPtr[i] = destCopyPtr[i] = *srcPtr++;

		destPtr += PF_BUFFER_WIDTH;							// next line
		destCopyPtr += PF_BUFFER_WIDTH;
	}while(--height);


					/************************/
					/* SEE IF DRAW THE MASK */
					/************************/

	if (maskFlag)
	{
		destPtr = (unsigned char *)(gPFMaskLookUpTable[rowS]+colS);
		if (tileNum&TILE_PRIORITY_MASK)
		{
			if (tileNum&TILE_PRIORITY_MASK2)						// see if do pixel accurate mask or just tile mask
			{

						/* DRAW PIXEL TILE MASK */

				destPtrB = (Ptr)destPtr;
				srcPtrB = (Ptr)copyOfSrc;
				height = TILE_SIZE;
				do
				{
					for (i=0; i < TILE_SIZE; i++)
					{
						pixel = *srcPtrB++;							// get pixel value

						if (gColorMaskArray[pixel])
							*destPtrB++ = 0xff;						// make xparent
						else
							*destPtrB++ = 0x00;						// make solid
					}
					destPtrB += PF_BUFFER_WIDTH-TILE_SIZE;			// next line
				} while(--height);

			}
							/* DRAW WHOLE TILE MASK */
			else
			{
				height = TILE_SIZE;
				do
				{
					for (i = 0; i < TILE_SIZE; i++)
						destPtr[i] = 0xff;
					destPtr += PF_BUFFER_WIDTH;			// next line
				} while (--height);
			}
		}
		else
		{
						/* CLEAR MASK HERE */

			height = TILE_SIZE;
			do
			{
				for (i = 0; i < TILE_SIZE; i++)
					destPtr[i] = 0;
				destPtr += PF_BUFFER_WIDTH;			// next line
			} while (--height);
		}
	}
}



/************************ DRAW A TILE : SIMPLE ***********************/
//
// This simple version doesnt worry about masks at all.
// Assumes that tileNum has ALREADY BEEN FILTERED!
//

void DrawATile_Simple(unsigned short tileNum, short row, short col)
{
uint8_t		*destPtr,*srcPtr,*destCopyPtr;
Ptr			destStartPtr,destCopyStartPtr;
unsigned long	rowS,colS;								// shifted version of row & col

					/* CALC DEST POINTERS */

	destStartPtr = (Ptr)(gPFLookUpTable[rowS = row<<TILE_SIZE_SH]+(colS = col<<TILE_SIZE_SH));
	destCopyStartPtr = (Ptr)(gPFCopyLookUpTable[rowS]+colS);

					/* CALC TILE DEFINITION ADDR */

	GAME_ASSERT(HandleBoundsCheck(gTileSetHandle, (Ptr) gTileXlatePtr));
	GAME_ASSERT(HandleBoundsCheck(gTileSetHandle, (Ptr) &gTileXlatePtr[tileNum]));

	srcPtr = (uint8_t *)( gTilesPtr + ((long)(gTileXlatePtr[tileNum]) << (TILE_SIZE_SH*2)) );
	destPtr = (uint8_t *)destStartPtr;
	destCopyPtr = (uint8_t *)destCopyStartPtr;

	GAME_ASSERT(HandleBoundsCheck(gTileSetHandle, (Ptr) srcPtr));

						/* DRAW THE TILE */

	for (int y = 0; y < TILE_SIZE; y++)
	{
		memcpy(destPtr,		srcPtr,	TILE_SIZE);
		memcpy(destCopyPtr,	srcPtr,	TILE_SIZE);
		destPtr		+= PF_BUFFER_WIDTH;			// next line
		destCopyPtr	+= PF_BUFFER_WIDTH;
		srcPtr		+= TILE_SIZE;
	}
}



/********************* DISPLAY PLAYFIELD ***************/
//
// Dump Current playfield area to the screen
//

void DisplayPlayfield(void)
{
Ptr			destPtr,srcPtr;
long		top,left,width;
unsigned long	height;
long		numSegments,seg;
Ptr			destPtrs[4];
Ptr			srcPtrs[4];
unsigned long	heights[4];
long		widths[4];
long		srcAdd,destAdd;
long		method;

	left	= Long_PositiveModulo(gTweenedScrollX + gShakeyScreenOffsetX, PF_BUFFER_WIDTH);		// get PF buffer pixel coords to start @
	top		= Long_PositiveModulo(gTweenedScrollY + gShakeyScreenOffsetY, PF_BUFFER_HEIGHT);

	if ((left+(PF_WINDOW_WIDTH-1)) > PF_BUFFER_WIDTH)		// see if 2 horiz segments
	{

						/* 2 HORIZ SEGMENTS */

		if ((top+(PF_WINDOW_HEIGHT-1)) > PF_BUFFER_HEIGHT)	// see if 2 vertical segments
		{
			method = 0;
			numSegments = 4;
			widths[0] = widths[2] = (PF_BUFFER_WIDTH-left);
			heights[0] = heights[1] = PF_BUFFER_HEIGHT-top;
			destPtrs[1] = (destPtrs[0] = (Ptr)(gScreenLookUpTable[PF_WINDOW_TOP]+
								+PF_WINDOW_LEFT)) + widths[0];

			widths[1] = widths[3] = (PF_WINDOW_WIDTH-widths[0]);
			heights[2] = heights[3] = PF_WINDOW_HEIGHT-heights[0];

			srcPtrs[2] = (Ptr)(gPFLookUpTable[0]+left);

			destPtrs[3] = (Ptr)((destPtrs[2] = (Ptr)(gScreenLookUpTable[PF_WINDOW_TOP+heights[0]]+PF_WINDOW_LEFT))+widths[0]);
			srcPtrs[3] = (Ptr)(gPFLookUpTable[0]);
			srcPtrs[0] = (srcPtrs[1] = (Ptr)(gPFLookUpTable[top])) + left;
		}
		else														// 1 vertical segment
		{
			method = 1;
			numSegments = 2;
			srcPtrs[0] = (Ptr)((srcPtrs[1] = (Ptr)(gPFLookUpTable[top]))+left);
			widths[0] = PF_BUFFER_WIDTH-left;
			heights[0] = heights[1] = PF_WINDOW_HEIGHT;

			destPtrs[1] = (Ptr)((destPtrs[0] = (Ptr)(gScreenLookUpTable[PF_WINDOW_TOP]+
								PF_WINDOW_LEFT))+widths[0]);
			widths[1] = (PF_WINDOW_WIDTH-widths[0]);
		}
	}
					/* ONLY 1 HORIZ SEGMENT */
	else
	{
		if ((top+(PF_WINDOW_HEIGHT-1)) > PF_BUFFER_HEIGHT)			// see if 2 vertical segments
		{
			method = 2;
			numSegments = 2;
			destPtrs[0] = (Ptr)(gScreenLookUpTable[PF_WINDOW_TOP]+
								PF_WINDOW_LEFT);
			widths[0] = widths[1] = PF_WINDOW_WIDTH;
			heights[1] = PF_WINDOW_HEIGHT-(heights[0] = PF_BUFFER_HEIGHT-top);

			destPtrs[1] = (Ptr)(gScreenLookUpTable[PF_WINDOW_TOP+heights[0]]+
								PF_WINDOW_LEFT);
			srcPtrs[1] = (Ptr)(gPFLookUpTable[0]+left);
			srcPtrs[0] = (Ptr)(gPFLookUpTable[top]+left);
		}
		else														// 1 vertical segment
		{
			method = 3;
			numSegments = 1;
			destPtrs[0] = (Ptr)(gScreenLookUpTable[PF_WINDOW_TOP]+
								PF_WINDOW_LEFT);
			srcPtrs[0] = (Ptr)gPFLookUpTable[top]+left;
			widths[0] = PF_WINDOW_WIDTH;
			heights[0] = PF_WINDOW_HEIGHT;
		}
	}

					/**********************************/
					/* SPECIAL CASE METHOD 0 / 4 SEGS */
					/**********************************/

	if (method == 0)
	{
		Ptr		destPtr2,srcPtr2;
		long	width2,srcAdd2,destAdd2;

		for (seg = 0; seg < 4; seg+=2)
		{
			destPtr = destPtrs[0+seg];
			srcPtr	= srcPtrs[0+seg];
			destPtr2 = destPtrs[1+seg];
			srcPtr2	= srcPtrs[1+seg];

			height = heights[0+seg];					// set height
			width = widths[0+seg];						// set width
			width2 = widths[1+seg];						// set width

			srcAdd = (PF_BUFFER_WIDTH)-width;			// calc line add (for normal display)
			destAdd = (VISIBLE_WIDTH)-width;
			srcAdd2 = (PF_BUFFER_WIDTH)-width2;			// calc line add (for normal display)
			destAdd2 = (VISIBLE_WIDTH)-width2;

						/* DO 1ST SEG */
			do
			{
				memcpy(destPtr, srcPtr, width);
				destPtr += width;
				srcPtr += width;


						/* DO 2ND SEG */

				memcpy(destPtr2, srcPtr2, width2);
				destPtr2 += width2;
				srcPtr2 += width2;

				srcPtr += srcAdd;							// next line
				destPtr += destAdd;
				srcPtr2 += srcAdd2;
				destPtr2 += destAdd2;
			} while (--height);
		}
	}
	else
	{
					/**********************************/
					/* NORMAL CASE METHOD 1..3        */
					/**********************************/

						/* COPY THE SEGMENTS */

		for (seg=0; seg < numSegments; seg++)
		{
			Ptr		destStartPtr,srcStartPtr;

			destStartPtr = destPtrs[seg];
			srcStartPtr	= srcPtrs[seg];

			height = heights[seg];						// set height
			width = widths[seg];						// set width

			srcAdd = PF_BUFFER_WIDTH;					// line bytes
			destAdd = VISIBLE_WIDTH;

			do
			{
				destPtr = destStartPtr;
				srcPtr = srcStartPtr;

				memcpy(destPtr, srcPtr, width);

				srcStartPtr += srcAdd;									// next line
				destStartPtr += destAdd;

			} while (--height);
		}
	}
}




/**************** UPDATE TILE ANIMATION **********************/
// Source port note: moved from TileAnim.c

void UpdateTileAnimation(void)
{
unsigned long	row;
unsigned short	newTile;
uint16_t	targetTile,*intPtr,*basePtr;
register long	col;
register long	x,y,animNum;
unsigned long 	origRow,origCol;

	origRow = gScrollRow % PF_TILE_HEIGHT;									// calc row in buffer
	origCol = gScrollCol % PF_TILE_WIDTH;									// calc col in buffer

	for (animNum = 0; animNum < gNumTileAnims; animNum++)
	{
						/* CHECK COUNTER */

		if ((gTileAnims[animNum].count -= gTileAnims[animNum].defPtr->speed) < 0)
		{
			gTileAnims[animNum].count = 0x100;								// reset counter

						/* SCAN VISIBLE AREA FOR TARGET TILE */

			targetTile = gTileAnims[animNum].defPtr->baseTile;				// get target basetile
			newTile = gTileAnims[animNum].defPtr->tileNums[gTileAnims[animNum].index];	// get tile to draw

			basePtr = (unsigned short *)&gPlayfield[gScrollRow][gScrollCol]; // get ptr to start of scan

			row = origRow;													// get modable row

			y = 0;
			do
			{
				intPtr = basePtr;											// get ptr to start of horiz scan
				col = origCol;
				x = PF_TILE_WIDTH;

				do
				{
					if ((*intPtr++ & TILENUM_MASK) == targetTile)
						DrawATile_Simple(newTile,row,col);

					if (++col >= PF_TILE_WIDTH)								// see if column wrap
						col = 0;
				} while (--x);

				if (++row >= (unsigned long) PF_TILE_HEIGHT)				// see if row wrap
					row = 0;

				basePtr += gPlayfieldTileWidth;								// next row in map
			} while (++y < PF_TILE_HEIGHT);


			y = ++gTileAnims[animNum].index;								// increment index
			if (y  >= gTileAnims[animNum].defPtr->numFrames)				// see if at end of sequence
				gTileAnims[animNum].index = 0;
		}
	}
}

