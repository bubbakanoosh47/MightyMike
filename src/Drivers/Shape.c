/****************************/
/*     SHAPE MANAGER        */
/* (c)1994 Pangea Software  */
/* By Brian Greenstone      */
/****************************/


/***************/
/* EXTERNALS   */
/***************/
#include "myglobals.h"
#include "window.h"
#include "playfield.h"
#include "object.h"
#include "misc.h"
#include "shape.h"
#include <string.h>
#include "externs.h"

/****************************/
/*    PROTOTYPES            */
/****************************/

static void DrawPFSprite(ObjNode *theNodePtr);
static void ErasePFSprite(ObjNode *theNodePtr);

/****************************/
/*    CONSTANTS             */
/****************************/

/**********************/
/*     VARIABLES      */
/**********************/

Handle	gShapeTableHandle[MAX_SHAPE_GROUPS] = {nil,nil,nil,nil,nil,nil,nil,nil};

Ptr		gSHAPE_HEADER_Ptrs[MAX_SHAPE_GROUPS][MAX_SHAPES_IN_FILE];			// holds ptr to each shape type's SHAPE_HEADER

static	short		gNumShapesInFile[MAX_SHAPE_GROUPS];

ObjNode	*gMostRecentShape = nil;


/************************ MAKE NEW SHAPE ***********************/
//
// MAKE NEW SHAPE OBJECT & RETURN PTR TO IT
//

ObjNode *MakeNewShape(long groupNum, long type, long subType, short x, short y, short z, void (*moveCall)(void), Boolean pfRelativeFlag)
{
ObjNode	*newSpritePtr;
Ptr		tempPtr;
int32_t	offset;

	if (groupNum >= MAX_SHAPE_GROUPS)										// see if legal group
		DoFatalAlert("Illegal shape group #");

	newSpritePtr = MakeNewObject(SPRITE_GENRE,x,y,z,moveCall);

	if (newSpritePtr == nil)								// return nil if no object
		return(nil);

	newSpritePtr->Type = type;								// set sprite type
	newSpritePtr->SubType = subType;						// set sprite subtype
	newSpritePtr->SpriteGroupNum = groupNum;				// set which sprite group it belongs to
	newSpritePtr->UpdateBoxFlag = !pfRelativeFlag;			// no box if PF relative
	newSpritePtr->DrawFlag = 								// init flags
	newSpritePtr->EraseFlag =
	newSpritePtr->AnimFlag =
	newSpritePtr->TileMaskFlag = true;						// use tile masks (if applicable)
	newSpritePtr->PFCoordsFlag = pfRelativeFlag;			// set if playfield coords or not
	newSpritePtr->YOffset.L = 0;							// assume no draw offset

	if (newSpritePtr->PFCoordsFlag)
	{
		newSpritePtr->drawBox.left =						// init draw box coords
		newSpritePtr->drawBox.right =
		newSpritePtr->drawBox.top =
		newSpritePtr->drawBox.bottom = 0;
	}
	else
	{
		newSpritePtr->drawBox.left		= x-2 + gScreenXOffset;
		newSpritePtr->drawBox.right		= x+2 + gScreenXOffset;
		newSpritePtr->drawBox.top		= y-2 + gScreenYOffset;
		newSpritePtr->drawBox.bottom	= y+2 + gScreenYOffset;
	}

	newSpritePtr->AnimSpeed = 								// init animation stuff
	newSpritePtr->AnimConst = 0x100;
	newSpritePtr->AnimLine =
	newSpritePtr->CurrentFrame =							// set to 0 just to be safe!!!
	newSpritePtr->AnimCount = 0;

	newSpritePtr->DZ = 0;

	newSpritePtr->ClipNum = CLIP_REGION_PLAYFIELD;			// assume clip to playfield

	newSpritePtr->SHAPE_HEADER_Ptr = tempPtr =
				gSHAPE_HEADER_Ptrs[groupNum][type];			// set ptr to SHAPE_HEADER

						/* INIT PTR TO ANIM_LIST */

	offset = *(int32_t*) (tempPtr+SHAPE_HEADER_ANIM_LIST);	// get offset to ANIM_LIST

	newSpritePtr->AnimsList = tempPtr+offset+2;				// set ptr to ANIM_LIST
															// but skip 1st word: #anims!!!!

	AnimateASprite(newSpritePtr);							// initialize anim by calling it

	gMostRecentShape = newSpritePtr;

	return (newSpritePtr);									// return ptr to new sprite node
}


/************************ LOAD SHAPE TABLE *****************/

void LoadShapeTable(const char* fileName, long groupNum)
{
					/* THE REAL WORK */

	if (gShapeTableHandle[groupNum] != nil)						// see if zap existing shapetable
	{
		DisposeHandle(gShapeTableHandle[groupNum]);
		memset(gSHAPE_HEADER_Ptrs[groupNum], 0, sizeof(gSHAPE_HEADER_Ptrs[groupNum]));
	}

	gShapeTableHandle[groupNum] = LoadPackedFile(fileName);

	Ptr shapeTablePtr = *gShapeTableHandle[groupNum];						// get ptr to shape table

	int32_t offsetToColorTable = UnpackI32BEInPlace(shapeTablePtr);			// get Color Table offset

	int16_t colorListSize = UnpackI16BEInPlace(shapeTablePtr + offsetToColorTable);	// # entries in color list
	GAME_ASSERT(colorListSize >= 0 && colorListSize <= 256);

#if 0
	/****************************** BUILD SHAPE PALETTE ********************/
	// Source port note: Mighty Mike always sets palette colors from image files, never from shape
	// files. Furthermore, the game's shape files never seem to actually define palette colors.

	if (usePalFlag)															// get shape table's pal going
	{
				/* BYTESWAP COLORS IN PALETTE */

		RGBColor* colorEntryPtr = (RGBColor *)(shapeTablePtr + offsetToColorTable + 2);	// point to color list
		ByteswapStructs("3H", sizeof(RGBColor), colorListSize, colorEntryPtr);

				/* BUILD THE PALETTE */

		for (int i = 0; i < colorListSize; i++)
		{
			RGBColor rgbColor = *colorEntryPtr++;					// get color
			gGamePalette[i] = RGBColorToU32(&rgbColor);				// set color
		}

				/* IF <256, THEN FORCE LAST COLOR TO BLACK */

		if (colorListSize < 256)
		{
			RGBColor black = {0,0,0};
			gGamePalette[255] = RGBColorToU32(&black);				// set color
		}
	}
#endif

 	/***************** CREATE SHAPE HEADER POINTERS ********************/
	//
	// This is called whenever a shape table is moved in memory or loaded
	//

	int32_t offsetToShapeList = UnpackI32BEInPlace(shapeTablePtr + SF_HEADER__SHAPE_LIST);		// get ptr to offset to SHAPE_LIST

	Ptr shapeList = shapeTablePtr + offsetToShapeList;				// get ptr to SHAPE_LIST

	gNumShapesInFile[groupNum] = UnpackI16BEInPlace(shapeList);		// get # shapes in the file
	shapeList += 2;

	int32_t* offsetsToShapeHeaders = (int32_t*) shapeList;			// get offset to SHAPE_HEADER_n
	UnpackIntsBE(4, gNumShapesInFile[groupNum], offsetsToShapeHeaders);

	for (int i = 0; i < gNumShapesInFile[groupNum]; i++)
	{
		Ptr shapeBase = shapeTablePtr + offsetsToShapeHeaders[i];

		gSHAPE_HEADER_Ptrs[groupNum][i] = shapeBase;	// save ptr to SHAPE_HEADER

		int32_t offsetToFrameList	= UnpackI32BEInPlace(shapeBase + 2);
		int16_t numFrames			= UnpackI16BEInPlace(shapeBase + offsetToFrameList);
		int32_t* offsetsToFrameData	= (int32_t*) (shapeBase + offsetToFrameList + 2);
		UnpackIntsBE(4, numFrames, offsetsToFrameData);

		for (int f = 0; f < numFrames; f++)
		{
			Ptr frameBase = shapeBase + offsetsToFrameData[f];

			UnpackStructs(">hhhhll", 16, 1, frameBase);	// See struct FrameHeader
		}

		int32_t offsetToAnimList	= UnpackI32BEInPlace(shapeBase + 6);  // base+SHAPE_HEADER_ANIM_LIST
		int16_t numAnims			= UnpackI16BEInPlace(shapeBase + offsetToAnimList);
		int32_t* offsetsToAnimData	= (int32_t*) (shapeBase + offsetToAnimList + 2);
		UnpackIntsBE(4, numAnims, offsetsToAnimData);

		for (int a = 0; a < numAnims; a++)
		{
			Ptr animBase = shapeBase + offsetsToAnimData[a];

			uint8_t numCommands = animBase[0];		// aka "AnimLine"

			UnpackIntsBE(2, numCommands*2, animBase+1);
			/*
			for (int cmd = 0; cmd < numCommands; cmd++)
			{
				Ptr commandBase = animBase + 1 + 4*cmd;
				int16_t opcode	= Byteswap16Signed(commandBase + 0);
				int16_t operand	= Byteswap16Signed(commandBase + 2);
			}
			*/
		}

//		printf("Num Anims: %d    Num Frames: %d\n", numAnims, numFrames);
	}
}

/************************ GET FRAME HEADER ********************/

const FrameHeader* GetFrameHeader(
		long groupNum,
		long shapeNum,
		long frameNum,
		const uint8_t** outPixelPtr,
		const uint8_t** outMaskPtr)
{
	const uint8_t*		shapePtr;
	const FrameList*	fl;
	const FrameHeader*	fh;

	GAME_ASSERT_MESSAGE(groupNum < MAX_SHAPE_GROUPS, "Illegal Group #");
	GAME_ASSERT_MESSAGE(shapeNum < gNumShapesInFile[groupNum], "Illegal Shape #");

			/* GET FRAME HEADER */

	shapePtr = (const uint8_t*) gSHAPE_HEADER_Ptrs[groupNum][shapeNum];		// get ptr to SHAPE_HEADER
	GAME_ASSERT(shapePtr);

	int32_t offsetToFrameList = *(int32_t*) (shapePtr+2);					// get ptr to FRAME_LIST
	fl = (const FrameList*) (shapePtr + offsetToFrameList);

	GAME_ASSERT_MESSAGE(frameNum < fl->numFrames, "Illegal Frame #");		// get ptr to FRAME_HEADER
	fh = (const FrameHeader*) (shapePtr + fl->frameHeaderOffsets[frameNum]);

	GAME_ASSERT(HandleBoundsCheck(gShapeTableHandle[groupNum], (Ptr) fh));

			/* STORE PTR TO PIXEL DATA */

	if (outPixelPtr)
	{
		*outPixelPtr = shapePtr + fh->pixelOffset;
		GAME_ASSERT(HandleBoundsCheck(gShapeTableHandle[groupNum], (Ptr) *outPixelPtr));
	}

			/* STORE PTR TO MASK DATA */

	if (outMaskPtr)
	{
		*outMaskPtr = shapePtr + fh->maskOffset;
		GAME_ASSERT(HandleBoundsCheck(gShapeTableHandle[groupNum], (Ptr) *outMaskPtr));
	}

			/* RETURN PTR TO FRAME HEADER */

	return fh;
}

/************************ DRAW FRAME TO GENERIC BUFFER ********************/

static void DrawFrameToBuffer(
		long x,
		long y,
		long groupNum,
		long shapeNum,
		long frameNum,
		bool mask,
		uint8_t* destBuffer,
		int destBufferWidth,
		int destBufferHeight
		)
{
					/* CALC ADDRESS OF FRAME TO DRAW */

	const uint8_t*	pixelData	= nil;
	const uint8_t*	maskData	= nil;

	const FrameHeader* fh = GetFrameHeader(
			groupNum,
			shapeNum,
			frameNum,
			&pixelData,
			mask? &maskData: nil
	);

	x += fh->x;										// use position offsets
	y += fh->y;

	x += gScreenXOffset;							// global centering offset
	y += gScreenYOffset;

	if (x < 0 || x >= destBufferWidth ||			// see if out of bounds
		y < 0 || y >= destBufferHeight)
		return;

	uint8_t* destPtr = destBuffer + y*destBufferWidth + x;

						/* DO THE DRAW */

	if (!mask)
	{
		for (int row = fh->height; row; row--)
		{
			memcpy(destPtr, pixelData, fh->width);

			destPtr += destBufferWidth;				// next row
			pixelData += fh->width;
		}
	}
	else
	{
		const uint8_t* srcPtr	= pixelData;
		const uint8_t* maskPtr	= maskData;

		for (int row = fh->height; row; row--)
		{
			uint8_t* destPtr2 = destPtr;			// get line start ptr

			for (int col = fh->width; col; col--)
			{
				*destPtr2 = (*destPtr2 & *maskPtr) | (*srcPtr);
				destPtr2++;
				maskPtr++;
				srcPtr++;
			}

			destPtr += destBufferWidth;			// next row
		}
	}
}

/************************ DRAW FRAME TO SCREEN ********************/
//
// NOTE: draws directly to the screen
//
// INPUT: x/y = LOCAL screen coords, not global!!!
//

void DrawFrameToScreen(long x,long y,long groupNum,long shapeNum,long frameNum)
{
	DrawFrameToBuffer(
			x,
			y,
			groupNum,
			shapeNum,
			frameNum,
			true,
			gIndexedFramebuffer,
			VISIBLE_WIDTH,
			VISIBLE_HEIGHT
	);
}

/************************ DRAW FRAME TO SCREEN: NO MASK  ********************/
//
// NOTE: draws directly to the screen
//
// INPUT: x/y = LOCAL screen coords, not global!!!
//

void DrawFrameToScreen_NoMask(long x,long y,long groupNum,long shapeNum,long frameNum)
{
	DrawFrameToBuffer(
			x,
			y,
			groupNum,
			shapeNum,
			frameNum,
			false,
			gIndexedFramebuffer,
			VISIBLE_WIDTH,
			VISIBLE_HEIGHT
	);
}

/************************ DRAW FRAME TO BACKGROUND BUFFER ********************/

void DrawFrameToBackground(long x,long y,long groupNum,long shapeNum,long frameNum)
{
	DrawFrameToBuffer(
			x,
			y,
			groupNum,
			shapeNum,
			frameNum,
			true,
			(uint8_t*) *gBackgroundHandle,
			OFFSCREEN_WIDTH,
			OFFSCREEN_HEIGHT
	);
}

/************************* ZAP SHAPE TABLE ***************************/
//
// groupNum = $ff means zap all
//

void ZapShapeTable(long groupNum)
{
long	i,i2;

	if (groupNum == 0xff)
	{
		i = 0;
		i2 = MAX_SHAPE_GROUPS-1;
	}
	else
	{
		i = groupNum;
		i2 = groupNum;
	}

	for ( ; i<=i2; i++)
	{
		if (gShapeTableHandle[i] != nil)
		{
			DisposeHandle(gShapeTableHandle[i]);
			gShapeTableHandle[i] = nil;

			// Clear pointers to shapes so the game will segfault if inadvertantly reusing zombie shapes
			memset(gSHAPE_HEADER_Ptrs[i], 0, sizeof(gSHAPE_HEADER_Ptrs[i]));
		}
	}
}




/************************** CHECK FOOT PRIORITY ****************************/

bool CheckFootPriority(long x, long y, long width)
{
long	col,row;
long	x2;


	if (y < 0 || y >= gPlayfieldHeight || x < 0 || x >= gPlayfieldWidth)		// check for bounds error
		return(false);

	row = y >> TILE_SIZE_SH;
	col = x >> TILE_SIZE_SH;
	x2 = (x+width) >> TILE_SIZE_SH;

	for (; col <= x2; col ++)
	{
		if (gPlayfield[row][col] & TILE_PRIORITY_MASK)
			return(true);
	}
	return(false);
}


/************************ DRAW A SPRITE ********************/
//
// Normal draw routine to draw a sprite Object
//

void DrawASprite(ObjNode *theNodePtr)
{
int32_t	width;
int32_t	height;
long	frameNum;
int32_t	x,y,offset;
int32_t clipTop,clipRight,clipLeft,clipBottom;
Rect	oldBox;
long	shapeNum,groupNum;
uint8_t*		destPtr;
uint8_t*		destStartPtr;
const uint8_t*	maskPtr;
const uint8_t*	srcPtr;

	if (theNodePtr->PFCoordsFlag)					// see if do special PF Draw code
	{
		DrawPFSprite(theNodePtr);
		return;
	}

	groupNum = theNodePtr->SpriteGroupNum;			// get shape group #
	shapeNum = theNodePtr->Type;					// get shape type
	frameNum = theNodePtr->CurrentFrame;			// get frame #
	x = (theNodePtr->X.Int);						// get short x coord
	y = (theNodePtr->Y.Int);						// get short y coord

					/* CALC ADDRESS OF FRAME TO DRAW */

	const FrameHeader* fh = GetFrameHeader(
			groupNum,
			shapeNum,
			frameNum,
			&srcPtr,
			&maskPtr
	);

	width = fh->width;								// get width
	height = fh->height;							// get height

	x += fh->x;										// use position offsets
	y += fh->y;

	x += gScreenXOffset;							// global centering offset
	y += gScreenYOffset;

	oldBox = theNodePtr->drawBox;						// remember old box
    
    clipLeft = (int32_t)gRegionClipLeft[theNodePtr->ClipNum];
    clipRight = (int32_t)gRegionClipRight[theNodePtr->ClipNum];
    clipTop = (int32_t)gRegionClipTop[theNodePtr->ClipNum];
    clipBottom = (int32_t)gRegionClipBottom[theNodePtr->ClipNum];

	if ((x < clipLeft) || ((x+width) >= clipRight) || ((y+height) <= clipTop) || (y >= clipBottom)) // see if out of bounds
        goto update;

	if ((y+height) > clipBottom)	// see if need to clip height
		height -= (y+height)-clipBottom;

	if (y < clipTop)			// see if need to clip TOP
	{
		offset = clipTop-y;
		y = clipTop;
		height -= offset;
		srcPtr += offset*width;
		maskPtr += offset*width;
	}

	if (theNodePtr->UpdateBoxFlag)						// see if using update regions
	{
		theNodePtr->drawBox.left = x;					// set drawn box
		theNodePtr->drawBox.right = x+(width<<2)-1;		// pixel widths
		theNodePtr->drawBox.top = y;
		theNodePtr->drawBox.bottom = y+height;
	}

	destStartPtr = gOffScreenLookUpTable[y] + x;		// calc draw addr

						/* DO THE DRAW */
	if (height <= 0)										// special check for illegal heights
		height = 1;

	do
	{
		destPtr = destStartPtr;								// get line start ptr

		for (int i = width; i; i--)
		{
			*destPtr = (*destPtr & *maskPtr) | (*srcPtr);
			destPtr++;
			srcPtr++;
			maskPtr++;
		}

		destStartPtr += OFFSCREEN_WIDTH;					// next row
	} while(--height);


					/* MAKE AN UPDATE REGION */
update:
	if (theNodePtr->UpdateBoxFlag)							// see if using update regions
	{
		if (oldBox.left > theNodePtr->drawBox.left)			// setup max box
			oldBox.left = theNodePtr->drawBox.left;

		if (oldBox.right < theNodePtr->drawBox.right)
			oldBox.right = theNodePtr->drawBox.right;

		if (oldBox.top > theNodePtr->drawBox.top)
			oldBox.top = theNodePtr->drawBox.top;

		if (oldBox.bottom < theNodePtr->drawBox.bottom)
			oldBox.bottom = theNodePtr->drawBox.bottom;

		AddUpdateRegion(oldBox,theNodePtr->ClipNum);
	}
}


/************************ ERASE A SPRITE ********************/

void EraseASprite(ObjNode *theNodePtr)
{
	if (theNodePtr->PFCoordsFlag)					// see if do special PF Erase code
	{
		ErasePFSprite(theNodePtr);
		return;
	}

	int x = theNodePtr->drawBox.left;				// get x coord
	int y = theNodePtr->drawBox.top;				// get y coord

	if ((x < 0) ||									// see if out of bounds
		(x >= OFFSCREEN_WIDTH) ||
		(y < 0) ||
		(y >= OFFSCREEN_HEIGHT))
			return;

	int width = (((theNodePtr->drawBox.right)-x)>>2)+2;		// in longs
	width <<= 2;											// convert back to bytes

	int height = (theNodePtr->drawBox.bottom)-y;


	uint8_t*		destPtr	= gOffScreenLookUpTable[y]+x;	// calc read/write addrs
	const uint8_t*	srcPtr	= gBackgroundLookUpTable[y]+x;

						/* DO THE ERASE */

	for (; height > 0; height--)
	{
		memcpy(destPtr, srcPtr, width);

		destPtr	+= OFFSCREEN_WIDTH;		// next row
		srcPtr	+= OFFSCREEN_WIDTH;
	}
}

/************************ DRAW PLAYFIELD SPRITE ********************/
//
// Draws shape obj into Playfield circular buffer
//

static void DrawPFSprite(ObjNode *theNodePtr)
{
int32_t	width,height;
uint8_t*			destStartPtr;
const uint8_t*		tileMaskStartPtr;
const uint8_t*		srcStartPtr;
const uint8_t*		originalSrcStartPtr;
const uint8_t*		maskStartPtr;
const uint8_t*		originalMaskStartPtr;
long	frameNum;
long	realWidth,topToClip,leftToClip;
long	shapeNum,groupNum,numHSegs;
int32_t originalY;
int32_t drawWidth;
Boolean	priorityFlag;
int32_t	x, y;

	groupNum = theNodePtr->SpriteGroupNum;				// get shape group #
	shapeNum = theNodePtr->Type;						// get shape type
	frameNum = theNodePtr->CurrentFrame;				// get frame #

					/* GET OBJECT POSITION (INTERPOLATED IN FRAMERATE-INDEPENDENT MODE)  */

	TweenObjectPosition(theNodePtr, &x, &y);

					/* CALC ADDRESS OF FRAME TO DRAW */

	const FrameHeader* fh = GetFrameHeader(
			groupNum,
			shapeNum,
			frameNum,
			(const uint8_t**) &srcStartPtr,
			(const uint8_t**) &maskStartPtr
	);

	drawWidth = realWidth = width = fh->width;		// get pixel width
	height = fh->height;							// get height
	x += fh->x;										// use position offsets (still global coords)
	y += fh->y;

				/************************/
				/*  CHECK IF VISIBLE    */
				/************************/

	if (((x+width) < gTweenedScrollX) || ((y+height) < gTweenedScrollY) ||
		(x >= (gTweenedScrollX+PF_BUFFER_WIDTH)) ||
		(y >= (gTweenedScrollY+PF_BUFFER_HEIGHT)))
	{
		theNodePtr->drawBox.left = 0;
		theNodePtr->drawBox.right = 0;
		theNodePtr->drawBox.top = 0;
		theNodePtr->drawBox.bottom = 0;
		return;
	}

					/***********************/
					/* CHECK VIEW CLIPPING */
					/***********************/

	if ((y+height) > (gTweenedScrollY+PF_BUFFER_HEIGHT))		// check vertical view clipping (bottom)
		height -= (y+height)-(gTweenedScrollY+PF_BUFFER_HEIGHT);

	if (y < gTweenedScrollY)									// check more vertical view clipping (top)
	{
		topToClip = (gTweenedScrollY-y);
		y += topToClip;
		height -= topToClip;
	}
	else
		topToClip = 0;

	if ((x+width) > (gTweenedScrollX+PF_BUFFER_WIDTH))			// check horiz view clipping (right)
	{
		width -= (x+width)-(gTweenedScrollX+PF_BUFFER_WIDTH);
		drawWidth = width;
	}

	if (x < gTweenedScrollX)									// check more horiz view clip (left)
	{
		leftToClip = (gTweenedScrollX-x);
		x += leftToClip;
		width -= leftToClip;
		drawWidth = width;
	}
	else
		leftToClip = 0;


	if (theNodePtr->TileMaskFlag)
	{
		// see if use priority masking
		// Source port note: pass in non-extrapolated foot Y to avoid blinking when an object is walking south towards a wall
		priorityFlag = CheckFootPriority(x, theNodePtr->Y.Int, drawWidth);
	}
	else
		priorityFlag = false;

	theNodePtr->drawBox.top = y = originalY =  (y % PF_BUFFER_HEIGHT);	// get PF buffer pixel coords to start @
	theNodePtr->drawBox.left = x = (x % PF_BUFFER_WIDTH);
	theNodePtr->drawBox.right = width;							// right actually = width
	theNodePtr->drawBox.bottom = height;

	if ((x+width) > PF_BUFFER_WIDTH)							// check horiz buffer clipping
	{
		width -= ((x+width)-PF_BUFFER_WIDTH);
		numHSegs = 2;											// 2 horiz segments
	}
	else
		numHSegs = 1;

	leftToClip += (topToClip*realWidth);
	srcStartPtr += leftToClip;
	maskStartPtr += leftToClip;
	originalSrcStartPtr = srcStartPtr;	 						// get ptr to PIXEL_DATA
	originalMaskStartPtr = maskStartPtr; 						// get ptr to MASK_DATA

	destStartPtr = gPFLookUpTable[y] + x;						// calc draw addr

						/* DO THE DRAW */


	if (!priorityFlag)
	{
		for (; numHSegs > 0; numHSegs--)
		{
			for (int drawHeight = 0; drawHeight < height; drawHeight++)
			{
				uint8_t*		destPtr		= destStartPtr;		// get line start ptr
				const uint8_t*	srcPtr		= srcStartPtr;
				const uint8_t*	maskPtr		= maskStartPtr;
				for (int i = width; i; i--)
				{
					*destPtr = (*destPtr & *maskPtr) | *srcPtr;
					destPtr++;
					maskPtr++;
					srcPtr++;
				}

				srcStartPtr += realWidth;						// next sprite line
				maskStartPtr += realWidth;						// next mask line

				if (++y >=  PF_BUFFER_HEIGHT)					// see if wrap buffer vertically
				{
					destStartPtr = gPFLookUpTable[0] + x;		// wrap to top
					y = 0;
				}
				else
					destStartPtr += PF_BUFFER_WIDTH;			// next buffer line
			}

			if (numHSegs == 2)
			{
				destStartPtr = gPFLookUpTable[y = originalY];	// set buff addr for segment #2
				x = 0;
				srcStartPtr = originalSrcStartPtr+width;
				maskStartPtr = originalMaskStartPtr+width;
				width = drawWidth-width;
			}
		}
	}
	else
	{
					/**************************/
					/* DRAW IT WITH TILE MASK */
					/**************************/

		tileMaskStartPtr = gPFMaskLookUpTable[y] + x;			// calc tilemask addr

		for (; numHSegs > 0; numHSegs--)
		{
			for (int drawHeight = 0; drawHeight < height; drawHeight++)
			{
				uint8_t* destPtr			= destStartPtr;		// get line start ptr
				const uint8_t* srcPtr		= srcStartPtr;
				const uint8_t* maskPtr		= maskStartPtr;
				const uint8_t* tileMaskPtr	= tileMaskStartPtr;

				for (int i = width; i; i--)						// draw remaining pixels
				{
					*destPtr =  (*destPtr & (*maskPtr | *tileMaskPtr)) |
								 (*srcPtr & (*tileMaskPtr ^ 0xff));
					destPtr++;
					maskPtr++;
					srcPtr++;
					tileMaskPtr++;
				}

				srcStartPtr += realWidth;						// next sprite line
				maskStartPtr += realWidth;						// next mask line

				if (++y >=  PF_BUFFER_HEIGHT)					// see if wrap buffer vertically
				{
					destStartPtr = gPFLookUpTable[0] + x;	// wrap to top
					tileMaskStartPtr = gPFMaskLookUpTable[0] + x;
					y = 0;
				}
				else
				{
					destStartPtr += PF_BUFFER_WIDTH;			// next buffer line
					tileMaskStartPtr += PF_BUFFER_WIDTH;
				}
			}

			if (numHSegs == 2)
			{
				destStartPtr = gPFLookUpTable[originalY];		// set buff addr for segment #2
				tileMaskStartPtr = gPFMaskLookUpTable[y = originalY];
				x = 0;
				srcStartPtr = originalSrcStartPtr+width;
				maskStartPtr = originalMaskStartPtr+width;
				width = drawWidth-width;
			}
		}
	}
}

/************************ ERASE PLAYFIELD SPRITE ********************/

static void ErasePFSprite(ObjNode *theNodePtr)
{
long	width,height,drawWidth,y;
uint8_t*		destPtr;
const uint8_t*	srcPtr;
long	x;
long	numHSegs;
long	originalY;

	x = theNodePtr->drawBox.left;					// remember area in the drawbox
	drawWidth = width = theNodePtr->drawBox.right;	// right actually = width
	originalY = y = theNodePtr->drawBox.top;
	height = theNodePtr->drawBox.bottom;

	if ((height <= 0) || (width <= 0))							// see if anything there
		return;

	if ((x+width) >= PF_BUFFER_WIDTH)				// check horiz clipping
	{
		width -= ((x+width)-PF_BUFFER_WIDTH);
		numHSegs = 2;								// 2 horiz segments
	}
	else
		numHSegs = 1;


	destPtr = gPFLookUpTable[y] + x;				// calc draw addr
	srcPtr = gPFCopyLookUpTable[y] + x;				// calc source addr

						/* DO THE ERASE */

	for (; numHSegs > 0; numHSegs--)
	{
		for (int drawHeight = 0; drawHeight < height; drawHeight++)
		{
			memcpy(destPtr, srcPtr, width);			// erase segment

			if (++y >=  PF_BUFFER_HEIGHT)			// see if wrap buffer vertically
			{
				destPtr = gPFLookUpTable[0] + x;	// wrap to top
				srcPtr = gPFCopyLookUpTable[0] + x;
				y = 0;
			}
			else
			{
				destPtr += PF_BUFFER_WIDTH;			// next buffer line
				srcPtr += PF_BUFFER_WIDTH;
			}
		}

		if (numHSegs == 2)
		{
			destPtr = gPFLookUpTable[originalY];	// set buff addr for segment #2
			srcPtr = gPFCopyLookUpTable[originalY];
			y = originalY;
			x = 0;
			width = drawWidth-width;
		}
	}
}

