// SETTINGS SCREEN
// (C) 2021 Iliyas Jorio
// This file is part of Mighty Mike. https://github.com/jorio/mightymike

/****************************/
/*    EXTERNALS             */
/****************************/

#include <stdio.h>
#include "myglobals.h"
#include "objecttypes.h"
#include "window.h"
#include "picture.h"
#include "playfield.h"
#include "object.h"
#include "misc.h"
#include "sound2.h"
#include "shape.h"
#include "io.h"
#include "main.h"
#include "input.h"
#include "version.h"
#include "externs.h"
#include "font.h"
#include "version.h"
#include <SDL.h>
#include <ctype.h>
#include <string.h>

#define MAX_CHOICES 16
#define MAX_ENTRIES_PER_MENU 25

#if !(OSXPPC) && !(GLRENDER)
#define EXPOSE_DITHERING 1
#else
#define EXPOSE_DITHERING 1
#endif

/****************************/
/*    PROTOTYPES            */
/****************************/

typedef enum
{
	kMenuItem_END_SENTINEL,
	kMenuItem_Label,
	kMenuItem_Action,
	kMenuItem_Submenu,
	kMenuItem_Separator,
	kMenuItem_Cycler,
	kMenuItem_KeyBinding,
	kMenuItem_PadBinding,
} MenuItemType;

typedef struct MenuItem
{
	MenuItemType			type;

	union
	{
		const char*			label;

		struct
		{
			const char*		caption;
			void			(*callback)(void);
		} button;

		struct
		{
			const char*		caption;
			struct MenuItem* menu;
		} submenu;

		struct
		{
			const char*		caption;
			void			(*callback)(void);
			Byte*			valuePtr;
			unsigned int	numChoices;
			const char*		choices[MAX_CHOICES];
		} cycler;

		int 				kb;
	};
} MenuItem;

static int MakeTextAtRowCol(const char* label, int row, int col, int flags);
static void DeleteTextAtRowCol(int row, int col);
static const char* GetKeyBindingName(int row, int col);
static const char* GetPadBindingName(int row, int col);
static void OnDone(void);
static void OnChangeFullscreenMode(void);
static void OnChangePlayfieldSizeViaSettings(void);
static void OnChangeDebugInfoInTitleBar(void);
static void OnResetKeys(void);
static void OnResetGamepad(void);
static void LayOutMenu(MenuItem* menu);
static void DrawDitheringPattern(void);

/****************************/
/*    CONSTANTS             */
/****************************/

static const char* kInputNeedCaptions[NUM_REMAPPABLE_NEEDS] =
{
	[kNeed_Up				] = "go up",
	[kNeed_Down				] = "go down",
	[kNeed_Left				] = "go left",
	[kNeed_Right			] = "go right",
	[kNeed_Attack			] = "attack",
	[kNeed_NextWeapon		] = "next weapon",
	[kNeed_PrevWeapon		] = "prev weapon",
	[kNeed_Radar			] = "bunny radar",
	[kNeed_ToggleMusic		] = "toggle music",
};

static const int kColumnX[] = { 64, 300, 475, 550 };
static const int kRowY0 = 90-24*2;
static const int kRowHeight = 24;

static const char* kUnboundCaption = "---";

const int16_t kJoystickDeadZone_BindingThreshold	= (75 * 32767 / 100);

/****************************/
/*    MENU CONTENTS         */
/****************************/

#pragma mark - Menu Contents

static MenuItem gVideoMenu[] =
{
	{ .type = kMenuItem_Label, .label = " VIDEO   SETTINGS" },
	{ .type = kMenuItem_Separator },
	{
		.type = kMenuItem_Cycler, .cycler =
		{
			.caption = "playfield size",
			.callback = OnChangePlayfieldSizeViaSettings,
			.valuePtr = &gGamePrefs.pfSize,
#if OSXPPC
			.numChoices = 2,		// don't expose extended
#else
			.numChoices = 3,
#endif
			.choices = { "small, 68k original", "medium, ppc original", "extended, widescreen" },
		}
	},
	{
		.type = kMenuItem_Cycler, .cycler =
		{
			.caption = "frame rate",
			.callback = nil,
			.valuePtr = &gGamePrefs.uncappedFramerate,
			.numChoices = 2,
			.choices = { "32 fps, like original", "smooth" },
		}
	},
	{ .type = kMenuItem_Separator },
	{
		.type = kMenuItem_Cycler, .cycler =
		{
			.caption = "display mode",
			.callback = OnChangeFullscreenMode,
			.valuePtr = &gGamePrefs.displayMode,
#if OSXPPC
			.numChoices = 2,
			.choices = {"windowed", "fullscreen"},
#else
			.numChoices = 3,
			.choices = {"windowed", "fullscreen, crisp", "fullscreen, stretched"},
#endif
		},
	},

	{
		.type = kMenuItem_Cycler, .cycler =
		{
			.caption = "windowed zoom",
			.valuePtr = &gGamePrefs.windowedZoom,
			.callback = OnChangePlayfieldSizeViaSettings,
			.numChoices = 3,  // set dynamically
			.choices =
			{
				"automatic",
				"1x",
				"2x",
				"3x",
				"4x",
				"5x",
				"6x",
				"7x",
				"8x",
				"9x",
				"10x",
				"11x",
				"12x",
				"13x",
				"14x",
				"15x"
			},
		}
	},

#if !(__APPLE__)
	{
		.type = kMenuItem_Cycler, .cycler =
		{
			.caption = "monitor",
			.valuePtr = &gGamePrefs.preferredDisplay,
			.callback = OnChangeFullscreenMode,
			.numChoices = 1,  // set dynamically
			.choices =
			{
				"monitor 1",
				"monitor 2",
				"monitor 3",
				"monitor 4",
				"monitor 5",
				"monitor 6",
				"monitor 7",
				"monitor 8",
				"monitor 9",
				"monitor 10",
				"monitor 11",
				"monitor 12",
				"monitor 13",
				"monitor 14",
				"monitor 15",
				"monitor 16",
			},
		}
	},
#endif

	{ .type = kMenuItem_Separator },

#if EXPOSE_DITHERING
	{
		.type = kMenuItem_Cycler, .cycler =
		{
			.caption = "shadow dithering",
			.callback = nil,
			.valuePtr = &gGamePrefs.filterDithering,
			.numChoices = 2,
			.choices = { "   raw", "   filtered" },
		}
	},
#endif

	{
		.type = kMenuItem_Cycler, .cycler =
		{
			.caption = "color correction",
			.callback = SetPaletteColorCorrection,
			.valuePtr = &gGamePrefs.colorCorrection,
			.numChoices = 2,
			.choices = { "no", "yes" },
		}
	},

	{ .type = kMenuItem_Action, .button = { .caption = "done", .callback = OnDone } },

	{ .type = kMenuItem_END_SENTINEL },
};

static MenuItem gAudioMenu[] =
{
	{ .type = kMenuItem_Label, .label = " AUDIO   SETTINGS" },
	{ .type = kMenuItem_Separator },
	{
		kMenuItem_Cycler,
		.cycler =
		{
			.caption = "music",
			.callback = OnToggleMusic,
			.valuePtr = &gGamePrefs.music,
			.numChoices = 2,
			.choices = { "no", "yes" },
		}
	},
	{
		kMenuItem_Cycler,
		.cycler =
		{
			.caption = "sound effects",
			.valuePtr = &gGamePrefs.soundEffects,
			.numChoices = 2,
			.choices = { "no", "yes" },
		}
	},
	{
		.type = kMenuItem_Cycler,
		.cycler =
		{
			.caption = "audio quality",
			.callback = OnChangeAudioInterpolation,
			.valuePtr = &gGamePrefs.interpolateAudio,
			.numChoices = 2,
			.choices = { "raw", "interpolated" },
		}
	},
	{ .type = kMenuItem_Action, .button = { .caption = "done", .callback = OnDone } },
	{ .type = kMenuItem_END_SENTINEL },
};

static MenuItem gPresentationMenu[] =
{
	{ .type = kMenuItem_Label, .label = " PRESENTATION   SETTINGS" },
	{ .type = kMenuItem_Separator },
	{
		.type = kMenuItem_Cycler, .cycler =
		{
			.caption = "game title",
			.callback = nil,
			.valuePtr = &gGamePrefs.gameTitlePowerPete,
			.numChoices = 2,
			.choices = { "mighty mike", "power pete" },
		}
	},
	{
		.type = kMenuItem_Cycler, .cycler =
		{
			.caption = "loading screen",
			.callback = nil,
			.valuePtr = &gGamePrefs.thermometerScreen,
			.numChoices = 2,
			.choices = { "instantaneous", "charging batteries" },
		}
	},
	{
		.type = kMenuItem_Cycler, .cycler =
		{
			.caption = "titlebar debug",
			.callback = OnChangeDebugInfoInTitleBar,
			.valuePtr = &gGamePrefs.debugInfoInTitleBar,
			.numChoices = 2,
			.choices = { "no", "yes" },
		}
	},
	{ .type = kMenuItem_Action, .button = { .caption = "done", .callback = OnDone } },
	{ .type = kMenuItem_END_SENTINEL },
};

static MenuItem gKeyboardMenu[] =
{
	{ .type = kMenuItem_Label, .label = " CONFIGURE   KEYBOARD" },
	{ .type = kMenuItem_Separator },
	{ .type = kMenuItem_KeyBinding, .kb = kNeed_Up },
	{ .type = kMenuItem_KeyBinding, .kb = kNeed_Down },
	{ .type = kMenuItem_KeyBinding, .kb = kNeed_Left },
	{ .type = kMenuItem_KeyBinding, .kb = kNeed_Right },
	{ .type = kMenuItem_KeyBinding, .kb = kNeed_Attack },
	{ .type = kMenuItem_KeyBinding, .kb = kNeed_PrevWeapon },
	{ .type = kMenuItem_KeyBinding, .kb = kNeed_NextWeapon },
	{ .type = kMenuItem_KeyBinding, .kb = kNeed_Radar },
	{ .type = kMenuItem_KeyBinding, .kb = kNeed_ToggleMusic },
	{ .type = kMenuItem_Separator },
	{ .type = kMenuItem_Action, .button = { .caption = "reset to defaults", .callback = OnResetKeys } },
	{ .type = kMenuItem_Action, .button = { .caption = "done", .callback = OnDone } },
	{ .type = kMenuItem_END_SENTINEL },
};

static MenuItem gGamepadMenu[] =
{
	{ .type = kMenuItem_Label, .label = " CONFIGURE   GAMEPAD" },
	{ .type = kMenuItem_Separator },
	{ .type = kMenuItem_PadBinding, .kb = kNeed_Attack },
	{ .type = kMenuItem_PadBinding, .kb = kNeed_PrevWeapon },
	{ .type = kMenuItem_PadBinding, .kb = kNeed_NextWeapon },
	{ .type = kMenuItem_PadBinding, .kb = kNeed_Radar },
	{ .type = kMenuItem_Separator },
	{ .type = kMenuItem_Action, .button = { .caption = "reset to defaults", .callback = OnResetGamepad } },
	{ .type = kMenuItem_Separator },
	{ .type = kMenuItem_Separator },
	{ .type = kMenuItem_Separator },
	{ .type = kMenuItem_Separator },
	{ .type = kMenuItem_Separator },
	{ .type = kMenuItem_Label, .label = "  TIP:  try aiming with the" },
	{ .type = kMenuItem_Label, .label = "                right analog stick!" },
	{ .type = kMenuItem_Separator },
	{ .type = kMenuItem_Action, .button = { .caption = "done", .callback = OnDone } },
	{ .type = kMenuItem_END_SENTINEL },
};

static MenuItem gRootMenu[] =
{
	{ .type = kMenuItem_Label, .label = " SETTINGS", },
	{ .type = kMenuItem_Separator },
	{ .type = kMenuItem_Submenu, .submenu = {.caption = "configure keyboard",	.menu = gKeyboardMenu} },
#if !(NOJOYSTICK)
	{ .type = kMenuItem_Submenu, .submenu = {.caption = "configure gamepad",	.menu = gGamepadMenu} },
#endif
	{ .type = kMenuItem_Submenu, .submenu = {.caption = "video",				.menu = gVideoMenu} },
	{ .type = kMenuItem_Submenu, .submenu = {.caption = "audio",				.menu = gAudioMenu} },
	{ .type = kMenuItem_Submenu, .submenu = {.caption = "presentation",			.menu = gPresentationMenu} },
	{ .type = kMenuItem_Separator },
	{ .type = kMenuItem_Action, .button = {.caption = "done", .callback = OnDone} },
	{ .type = kMenuItem_END_SENTINEL },
};

/****************************/
/*    VARIABLES             */
/****************************/

#pragma mark - Static Globals

static MenuItem* gMenu;
static int gNumMenuEntries;
static int gMenuRow = 0;
static int gKeyColumn = 0;
static int gPadColumn = 0;
static bool gAwaitingKeyPressForRebind = false;
static bool gExitMenu = false;

static int gMenuRowYs[MAX_ENTRIES_PER_MENU];

static int gLastRowOnRootMenu = -1;


/******************************************************************************/

/****************************/
/*    UTILITIES             */
/****************************/
#pragma mark - Utilities

static void ForceUpdateBackground(void)
{
	Rect r;
	r.left		= 0;
	r.top		= 0;
	r.right		= OFFSCREEN_WIDTH;
	r.bottom	= OFFSCREEN_HEIGHT;
	AddUpdateRegion(r, CLIP_REGION_PLAYFIELD);
}

static int FindRowControlling(Byte* valuePtr)
{
	for (int i = 0; i < MAX_ENTRIES_PER_MENU; i++)
	{
		const MenuItem* m = &gMenu[i];

		if (m->type == kMenuItem_END_SENTINEL)
		{
			return -1;
		}
		else if (m->type == kMenuItem_Cycler && m->cycler.valuePtr == valuePtr)
		{
			return i;
		}
	}

	return -1;
}

static void RemapRedToGray(int row)
{
	static bool remapTableReady = false;
	static uint8_t r[256];

	if (!remapTableReady)
	{
		for (int i = 0; i < 256; i++)
		{
			r[i] = i;
		}

		r[  7] = 246;
		r[ 14] = 245;
		r[ 21] = 246;
		r[ 28] = 247;
		r[ 35] = 247;
		r[107] = 250;
		r[143] = 253;
		r[179] = 253;
		r[215] = 248;
		r[216] = 248;
		r[217] = 249;
		r[218] = 250;
		r[219] = 251;
		r[220] = 252;
		r[221] = 253;
		r[222] = 253;
		r[223] = 254;
		r[224] = 254;
		remapTableReady = true;
	}

	uint8_t* pixels = (uint8_t*) *gBackgroundHandle;
	pixels += (gMenuRowYs[row] - 10) * OFFSCREEN_WIDTH;

	int maxRow = row + 24;

	for (; row < maxRow; row++)
		for (int x = 0; x < OFFSCREEN_WIDTH; x++)
		{
			*pixels = r[*pixels];
			pixels++;
		}
}

static void OnMenuEntered(void)
{
	if (gMenu == gVideoMenu)
	{
		{
			int row = FindRowControlling(&gGamePrefs.windowedZoom);
			GAME_ASSERT(row >= 0);

			int numChoices = 1 + GetMaxIntegerZoomForPreferredDisplay();
			if (numChoices > MAX_CHOICES)
				numChoices = MAX_CHOICES;
			gMenu[row].cycler.numChoices = numChoices;
		}

#if !(__APPLE__)
		{
			int row = FindRowControlling(&gGamePrefs.preferredDisplay);
			GAME_ASSERT(row >= 0);

			int numDisplays = SDL_GetNumVideoDisplays();
			if (numDisplays > MAX_CHOICES)
				numDisplays = MAX_CHOICES;
			gMenu[row].cycler.numChoices = numDisplays;
		}
#endif
	}
}

/****************************/
/*    CALLBACKS             */
/****************************/
#pragma mark - Callbacks

static void OnDone(void)
{
	ReadKeyboard();

	if (gAwaitingKeyPressForRebind)
	{
		PlaySound(SOUND_BADHIT);
		if (gMenu == gKeyboardMenu)
		{
			DeleteTextAtRowCol(gMenuRow, gKeyColumn + 1);
			MakeTextAtRowCol(GetKeyBindingName(gMenuRow, gKeyColumn), gMenuRow, 1 + gKeyColumn, kTextFlags_AsObject | kTextFlags_BounceUp);
		}
		else if (gMenu == gGamepadMenu)
		{
			DeleteTextAtRowCol(gMenuRow, gPadColumn + 1);
			MakeTextAtRowCol(GetPadBindingName(gMenuRow, gPadColumn), gMenuRow, 1 + gPadColumn, kTextFlags_AsObject | kTextFlags_BounceUp);
		}
		gAwaitingKeyPressForRebind = false;
	}
	else if (gMenu != gRootMenu)
	{
//		FadeOutGameCLUT();
		PlaySound(SOUND_SQUEEK);
		LayOutMenu(gRootMenu);
	}
	else
	{
		PlaySound(SOUND_SQUEEK);
		FadeOutGameCLUT();
		gExitMenu = true;
	}
}

static void OnChangeFullscreenMode(void)
{
#if !OSXPPC
	SetFullscreenMode(true);
#else
	int y = 340;
	MakeText("  N O T E: the new display mode will", kColumnX[0], y, 0, 0);
	MakeText("apply after restarting the game",    kColumnX[0], y+24, 0, 0);
	ForceUpdateBackground();
	DumpBackground();
#endif
}

static void OnChangePlayfieldSizeViaSettings(void)
{
	gScreenBlankedFlag = true;
	OnChangePlayfieldSize();
	SetScreenOffsetFor640x480();
	InitClipRegions();
	gScreenBlankedFlag = false;
	LayOutMenu(gMenu);//LayOutSettingsPageBackground();
}

static void OnChangeDebugInfoInTitleBar(void)
{
	SDL_SetWindowTitle(gSDLWindow, "Mighty Mike " PROJECT_VERSION);
}

static void OnResetKeys(void)
{
	for (int i = 0; i < NUM_CONTROL_NEEDS; i++)
	{
		memcpy(gGamePrefs.keys[i].key, kDefaultKeyBindings[i].key, sizeof(gGamePrefs.keys[i].key));
	}

	PlaySound(SOUND_FIREHOLE);
	LayOutMenu(gMenu);
}

static void OnResetGamepad(void)
{
	for (int i = 0; i < NUM_CONTROL_NEEDS; i++)
	{
		memcpy(gGamePrefs.keys[i].gamepad, kDefaultKeyBindings[i].gamepad, sizeof(gGamePrefs.keys[i].gamepad));
	}

	PlaySound(SOUND_FIREHOLE);
	LayOutMenu(gMenu);
}

/****************************/
/*    MOVE CALLS            */
/****************************/
#pragma mark - Move Calls

static void MoveCursor(void)
{
	int targetX;
	int targetY;

				/* FIND TARGET X, Y */

	MenuItem* entry = &gMenu[gMenuRow];

	targetY = gMenuRowYs[gMenuRow];

	switch (entry->type)
	{
		case kMenuItem_KeyBinding:
			targetX = kColumnX[1 + gKeyColumn];
			break;
		case kMenuItem_PadBinding:
			targetX = kColumnX[1 + gPadColumn];
			break;
		default:
			targetX = kColumnX[0];
			break;
	}

	targetX -= 20;
	targetY += 4;

				/* CALC DIFF X, Y */

	int diffX = (targetX << 16) - gThisNodePtr->X.L;
	int diffY = (targetY << 16) - gThisNodePtr->Y.L;

				/* CHANGE ANIM DEPENDING ON DIRECTION */

	long anim = gThisNodePtr->SubType;
	if (abs(diffX) < 0x8000 && abs(diffY) < 0x8000)
	{
	}
	else if (abs(diffX) > abs(diffY))
	{
		anim = diffX < 0 ? 3 : 1;
	}
	else
	{
		anim = diffY < 0 ? 2 : 0;
	}

	if (gThisNodePtr->SubType  != anim)
	{
		SwitchAnim(gThisNodePtr, anim);
		gThisNodePtr->AnimSpeed = 0x080;
	}

				/* MOVE IT */

	gThisNodePtr->X.L += diffX / 6;
	gThisNodePtr->Y.L += diffY / 6;
}

/****************************/
/*    SCANCODE STUFF        */
/****************************/
#pragma mark - Scancode stuff

static KeyBinding* GetBindingAtRow(int row)
{
	return &gGamePrefs.keys[gMenu[row].kb];
}

static const char* GetKeyBindingName(int row, int col)
{
	int16_t scancode = GetBindingAtRow(row)->key[col];

	switch (scancode)
	{
		case 0:							return kUnboundCaption;
		case SDL_SCANCODE_SEMICOLON:	return "semicolon";
		case SDL_SCANCODE_LEFTBRACKET:	return "l bracket";
		case SDL_SCANCODE_RIGHTBRACKET:	return "r bracket";
		case SDL_SCANCODE_SLASH:		return "slash";
		case SDL_SCANCODE_BACKSLASH:	return "backslash";
		case SDL_SCANCODE_GRAVE:		return "grave";
		case SDL_SCANCODE_MINUS:		return "minus";
		case SDL_SCANCODE_EQUALS:		return "equals";
		case SDL_SCANCODE_APOSTROPHE:	return "apostrophe";
		case SDL_SCANCODE_COMMA:		return "comma";
		case SDL_SCANCODE_PERIOD:		return "period";
		case SDL_SCANCODE_KP_PLUS:		return "keypad plus";
		case SDL_SCANCODE_KP_MINUS:		return "keypad minus";
		case SDL_SCANCODE_KP_DIVIDE:	return "keypad divide";
		case SDL_SCANCODE_KP_MULTIPLY:	return "keypad mult";
		case SDL_SCANCODE_KP_DECIMAL:	return "keypad decimal";
	}

#define kNameBufferLength 64
	static char nameBuffer[kNameBufferLength];

	const char* nameSource = SDL_GetScancodeName(scancode);
	for (int i = 0; i < kNameBufferLength - 1; i++)
	{
		char c = nameSource[i];
		if (c == '\0')
		{
			nameBuffer[i] = '\0';
			break;
		}
		else
		{
			nameBuffer[i] = tolower(c);
		}
	}
	nameBuffer[kNameBufferLength-1] = '\0';
	return nameBuffer;
#undef kNameBufferLength
}

static const char* GetPadBindingName(int row, int col)
{
	KeyBinding* kb = GetBindingAtRow(row);

	switch (kb->gamepad[col].type)
	{
		case kUnbound:
			return kUnboundCaption;

		case kButton:
			switch (kb->gamepad[col].id)
			{
				case SDL_CONTROLLER_BUTTON_INVALID:			return kUnboundCaption;
				case SDL_CONTROLLER_BUTTON_LEFTSHOULDER:	return "lb";
				case SDL_CONTROLLER_BUTTON_RIGHTSHOULDER:	return "rb";
				case SDL_CONTROLLER_BUTTON_LEFTSTICK:		return "ls";
				case SDL_CONTROLLER_BUTTON_RIGHTSTICK:		return "rs";
				default:
					return SDL_GameControllerGetStringForButton(kb->gamepad[col].id);
			}
			break;

		case kAxisPlus:
			switch (kb->gamepad[col].id)
			{
				case SDL_CONTROLLER_AXIS_LEFTX:				return "ls right";
				case SDL_CONTROLLER_AXIS_LEFTY:				return "ls down";
				case SDL_CONTROLLER_AXIS_RIGHTX:			return "rs right";
				case SDL_CONTROLLER_AXIS_RIGHTY:			return "rs down";
				case SDL_CONTROLLER_AXIS_TRIGGERLEFT:		return "lt";
				case SDL_CONTROLLER_AXIS_TRIGGERRIGHT:		return "rt";
				default:
					return SDL_GameControllerGetStringForAxis(kb->gamepad[col].id);
			}
			break;

		case kAxisMinus:
			switch (kb->gamepad[col].id)
			{
				case SDL_CONTROLLER_AXIS_LEFTX:				return "ls left";
				case SDL_CONTROLLER_AXIS_LEFTY:				return "ls up";
				case SDL_CONTROLLER_AXIS_RIGHTX:			return "rs left";
				case SDL_CONTROLLER_AXIS_RIGHTY:			return "rs up";
				case SDL_CONTROLLER_AXIS_TRIGGERLEFT:		return "lt";
				case SDL_CONTROLLER_AXIS_TRIGGERRIGHT:		return "rt";
				default:
					return SDL_GameControllerGetStringForAxis(kb->gamepad[col].id);
			}
			break;

		default:
			return "???";
	}
}


static void UnbindScancodeFromAllRemappableInputNeeds(int16_t sdlScancode)
{
	for (int row = 0; row < gNumMenuEntries; row++)
	{
		MenuItem* entry = &gMenu[row];

		if (entry->type != kMenuItem_KeyBinding)
			continue;

		KeyBinding* binding = GetBindingAtRow(row);

		for (int j = 0; j < KEYBINDING_MAX_KEYS; j++)
		{
			if (binding->key[j] == sdlScancode)
			{
				binding->key[j] = 0;
				DeleteTextAtRowCol(row, j+1);
				MakeTextAtRowCol(kUnboundCaption, row, j+1, kTextFlags_AsObject | kTextFlags_BounceUp);
			}
		}
	}
}

static void UnbindPadButtonFromAllRemappableInputNeeds(int8_t type, int8_t id)
{
	for (int row = 0; row < gNumMenuEntries; row++)
	{
		MenuItem* entry = &gMenu[row];

		if (entry->type != kMenuItem_PadBinding)
			continue;

		KeyBinding* binding = GetBindingAtRow(row);

		for (int j = 0; j < KEYBINDING_MAX_GAMEPAD_BUTTONS; j++)
		{
			if (binding->gamepad[j].type == type && binding->gamepad[j].id == id)
			{
				binding->gamepad[j].type = kUnbound;
				binding->gamepad[j].id = 0;
				DeleteTextAtRowCol(row, j+1);
				MakeTextAtRowCol(kUnboundCaption, row, j+1, kTextFlags_AsObject | kTextFlags_BounceUp);
			}
		}
	}
}


/****************************/
/*    MENU NAVIGATION       */
/****************************/
#pragma mark - Menu navigation

static void NavigateSettingEntriesVertically(int delta)
{
	bool skipEntry = false;
	do
	{
		gMenuRow += delta;
		gMenuRow = PositiveModulo(gMenuRow, (unsigned int)gNumMenuEntries);

		int kind = gMenu[gMenuRow].type;
		skipEntry = kind == kMenuItem_Separator || kind == kMenuItem_Label;
	} while (skipEntry);
	PlaySound(SOUND_SELECTCHIME);
}

static void NavigateButton(MenuItem* entry)
{
	if (GetNewNeedState(kNeed_UIConfirm))
	{
		if (entry->button.callback != OnDone)
			PlaySound(SOUND_GETPOW);

		if (entry->button.callback)
			entry->button.callback();
	}
}

static void NavigateSubmenuButton(MenuItem* entry)
{
	if (GetNewNeedState(kNeed_UIConfirm))
	{
		PlaySound(SOUND_FOOD);

		ReadKeyboard();	// flush keypresses

//		FadeOutGameCLUT();
		LayOutMenu(entry->submenu.menu);
	}
}

static void NavigateCycler(MenuItem* entry)
{
	int delta = 0;

	if (GetNewNeedState(kNeed_UILeft) || GetNewNeedState(kNeed_UIPrev))
		delta = -1;
	else if (GetNewNeedState(kNeed_UIRight) || GetNewNeedState(kNeed_UINext) || GetNewNeedState(kNeed_UIConfirm))
		delta = 1;

	if (delta != 0)
	{
		if (entry->cycler.valuePtr)
		{
			unsigned int value = (unsigned int)*entry->cycler.valuePtr;
			value = PositiveModulo(value + delta, entry->cycler.numChoices);
			*entry->cycler.valuePtr = value;
		}

		if (entry->cycler.callback)
			entry->cycler.callback();

		if (entry->cycler.valuePtr == &gGamePrefs.interpolateAudio)
			PlaySound(SOUND_COMEHERERODENT);
		else
			PlaySound(SOUND_GETPOW);

		if (entry->cycler.numChoices > 0)
		{
			DeleteTextAtRowCol(gMenuRow, 1);
			MakeTextAtRowCol(entry->cycler.choices[*entry->cycler.valuePtr], gMenuRow, 1, kTextFlags_AsObject | kTextFlags_BounceUp);
		}
	}
}

static void NavigateKeyBinding(MenuItem* entry)
{
	if (GetNewNeedState(kNeed_UILeft) || GetNewNeedState(kNeed_UIPrev))
	{
		gKeyColumn = PositiveModulo(gKeyColumn - 1, KEYBINDING_MAX_KEYS);
		PlaySound(SOUND_SELECTCHIME);
		return;
	}

	if (GetNewNeedState(kNeed_UIRight) || GetNewNeedState(kNeed_UINext))
	{
		gKeyColumn = PositiveModulo(gKeyColumn + 1, KEYBINDING_MAX_KEYS);
		PlaySound(SOUND_SELECTCHIME);
		return;
	}

	if (GetNewSDLKeyState(SDL_SCANCODE_DELETE) || GetNewSDLKeyState(SDL_SCANCODE_BACKSPACE))
	{
		gGamePrefs.keys[entry->kb].key[gKeyColumn] = 0;
		PlaySound(SOUND_PIESQUISH);
		PlaySound(SOUND_POP);

		DeleteTextAtRowCol(gMenuRow, gKeyColumn+1);
		MakeTextAtRowCol(kUnboundCaption, gMenuRow, gKeyColumn+1, kTextFlags_AsObject | kTextFlags_BounceUp);
		return;
	}

	if (GetNewSDLKeyState(SDL_SCANCODE_RETURN))
	{
		gAwaitingKeyPressForRebind = true;
		DeleteTextAtRowCol(gMenuRow, gKeyColumn+1);
		MakeTextAtRowCol("press a key!", gMenuRow, gKeyColumn+1, kTextFlags_AsObject | kTextFlags_BounceUp | kTextFlags_Jitter);
		return;
	}
}

static void NavigatePadBinding(MenuItem* entry)
{
	if (GetNewNeedState(kNeed_UILeft))
	{
		gPadColumn = PositiveModulo(gPadColumn - 1, KEYBINDING_MAX_GAMEPAD_BUTTONS);
		PlaySound(SOUND_SELECTCHIME);
		return;
	}

	if (GetNewNeedState(kNeed_UIRight))
	{
		gPadColumn = PositiveModulo(gPadColumn + 1, KEYBINDING_MAX_GAMEPAD_BUTTONS);
		PlaySound(SOUND_SELECTCHIME);
		return;
	}

	if (GetNewSDLKeyState(SDL_SCANCODE_DELETE) || GetNewSDLKeyState(SDL_SCANCODE_BACKSPACE))
	{
		gGamePrefs.keys[entry->kb].gamepad[gPadColumn].type = kUnbound;
		PlaySound(SOUND_PIESQUISH);
		PlaySound(SOUND_POP);

		DeleteTextAtRowCol(gMenuRow, gPadColumn+1);
		MakeTextAtRowCol(kUnboundCaption, gMenuRow, gPadColumn+1, kTextFlags_AsObject | kTextFlags_BounceUp);
		return;
	}

	if (GetNewNeedState(kNeed_UIConfirm))
	{
		while (GetNeedState(kNeed_UIConfirm))
		{
			UpdateInput();
			SDL_Delay(30);
		}

		gAwaitingKeyPressForRebind = true;
		DeleteTextAtRowCol(gMenuRow, gPadColumn+1);
		MakeTextAtRowCol("press button!", gMenuRow, gPadColumn+1, kTextFlags_AsObject | kTextFlags_BounceUp | kTextFlags_Jitter);
		return;
	}
}

static void NavigateMenu(void)
{
	if (GetNewNeedState(kNeed_UIBack))
		OnDone();

	if (GetNewNeedState(kNeed_UIUp))
		NavigateSettingEntriesVertically(-1);

	if (GetNewNeedState(kNeed_UIDown))
		NavigateSettingEntriesVertically(1);

	MenuItem* entry = &gMenu[gMenuRow];

	switch (entry->type)
	{
		case kMenuItem_Action:
			NavigateButton(entry);
			break;

		case kMenuItem_Submenu:
			NavigateSubmenuButton(entry);
			break;

		case kMenuItem_Cycler:
			NavigateCycler(entry);
			break;

		case kMenuItem_KeyBinding:
			NavigateKeyBinding(entry);
			break;

		case kMenuItem_PadBinding:
			NavigatePadBinding(entry);
			break;

		default:
			DoFatalAlert("Not supposed to be hovering on this menu item!");
			break;
	}
}

static void OnAwaitingKeyPress(void)
{
	if (GetNewSDLKeyState(SDL_SCANCODE_ESCAPE))
	{
		OnDone();
		return;
	}

	KeyBinding* kb = GetBindingAtRow(gMenuRow);

	if (gMenu == gKeyboardMenu)
	{
		for (int16_t scancode = 0; scancode < SDL_NUM_SCANCODES; scancode++)
		{
			if (GetNewSDLKeyState(scancode))
			{
				UnbindScancodeFromAllRemappableInputNeeds(scancode);
				kb->key[gKeyColumn] = scancode;
				DeleteTextAtRowCol(gMenuRow, gKeyColumn+1);
				MakeTextAtRowCol(GetKeyBindingName(gMenuRow, gKeyColumn), gMenuRow, gKeyColumn+1, kTextFlags_AsObject | kTextFlags_BounceUp);
				gAwaitingKeyPressForRebind = false;
				PlaySound(SOUND_COINS);
				return;
			}
		}
	}
	else if (gMenu == gGamepadMenu)
	{
		if (gSDLController)
		{
			if (SDL_GameControllerGetButton(gSDLController, SDL_CONTROLLER_BUTTON_START))
			{
				OnDone();
				return;
			}

			for (int8_t button = 0; button < SDL_CONTROLLER_BUTTON_MAX; button++)
			{
				switch (button)
				{
					case SDL_CONTROLLER_BUTTON_DPAD_UP:			// prevent binding those
					case SDL_CONTROLLER_BUTTON_DPAD_DOWN:
					case SDL_CONTROLLER_BUTTON_DPAD_LEFT:
					case SDL_CONTROLLER_BUTTON_DPAD_RIGHT:
						continue;
				}

				if (SDL_GameControllerGetButton(gSDLController, button))
				{
					UnbindPadButtonFromAllRemappableInputNeeds(kButton, button);
					kb->gamepad[gPadColumn].type = kButton;
					kb->gamepad[gPadColumn].id = button;
					DeleteTextAtRowCol(gMenuRow, gPadColumn+1);
					MakeTextAtRowCol(GetPadBindingName(gMenuRow, gPadColumn), gMenuRow, gPadColumn+1, kTextFlags_AsObject | kTextFlags_BounceUp);
					gAwaitingKeyPressForRebind = false;
					PlaySound(SOUND_COINS);
					return;
				}
			}

			for (int8_t axis = 0; axis < SDL_CONTROLLER_AXIS_MAX; axis++)
			{
				switch (axis)
				{
					case SDL_CONTROLLER_AXIS_LEFTX:				// prevent binding those
					case SDL_CONTROLLER_AXIS_LEFTY:
					case SDL_CONTROLLER_AXIS_RIGHTX:
					case SDL_CONTROLLER_AXIS_RIGHTY:
						continue;
				}

				int16_t axisValue = SDL_GameControllerGetAxis(gSDLController, axis);
				if (abs(axisValue) > kJoystickDeadZone_BindingThreshold)
				{
					int axisType = axisValue < 0 ? kAxisMinus : kAxisPlus;
					UnbindPadButtonFromAllRemappableInputNeeds(axisType, axis);
					kb->gamepad[gPadColumn].type = axisType;
					kb->gamepad[gPadColumn].id = axis;
					DeleteTextAtRowCol(gMenuRow, gPadColumn+1);
					MakeTextAtRowCol(GetPadBindingName(gMenuRow, gPadColumn), gMenuRow, gPadColumn+1, kTextFlags_AsObject | kTextFlags_BounceUp);
					gAwaitingKeyPressForRebind = false;
					PlaySound(SOUND_COINS);
					return;
				}
			}
		}
	}
}

/****************************/
/*    TEXT LAYOUT           */
/****************************/
#pragma mark - Text Layout

static long MakeStringID(int row, int col)
{
	return (row << 8) | (col);
}

static int MakeTextAtRowCol(const char* label, int row, int col, int flags)
{
	int x = kColumnX[col];
	int y = gMenuRowYs[row];
	return MakeText(label, x, y, flags, MakeStringID(row, col));
}

static void DeleteTextAtRowCol(int row, int col)
{
	DeleteText(MakeStringID(row, col));
}

/****************************/
/*    PAGE LAYOUT           */
/****************************/
#pragma mark - Page Layout

static void LayOutMenu(MenuItem* menu)
{
	bool enteringNewMenu = menu != gMenu;

	if (gMenu == gRootMenu)				// save position in root menu
		gLastRowOnRootMenu = gMenuRow;

	gMenu			= menu;
	gExitMenu		= false;
	gNumMenuEntries	= 0;

	int textObjectFlags = kTextFlags_AsObject;

	if (enteringNewMenu)
	{
		gMenuRow		= 2;
		textObjectFlags |= kTextFlags_BounceUp;

		if (menu == gRootMenu && gLastRowOnRootMenu >= 0)				// restore position in root menu
			gMenuRow = gLastRowOnRootMenu;
	}

	//DeleteAllObjects();
	DeleteAllText();
	EraseBackgroundBuffer();

#if 0
	// Color test
	for (int y = 0; y < OFFSCREEN_HEIGHT; y++)
		memset(*gBackgroundHandle + y*OFFSCREEN_WIDTH, y%256, OFFSCREEN_WIDTH);
#endif

	OnMenuEntered();

	int y = kRowY0;

	for (int row = 0; menu[row].type != kMenuItem_END_SENTINEL; row++)
	{
		if (menu[row + 1].type == kMenuItem_END_SENTINEL)
			y = 480-kRowY0;
		gMenuRowYs[row] = y;

		MenuItem* entry = &menu[row];

		switch (entry->type)
		{
			case kMenuItem_Separator:
				break;

			case kMenuItem_Label:
				MakeTextAtRowCol(entry->label, row, 0, 0);
				if (row != 0)
					RemapRedToGray(row);
				break;

			case kMenuItem_Action:
				MakeTextAtRowCol(entry->button.caption, row, 0, 0);
				break;

			case kMenuItem_Submenu:
				MakeTextAtRowCol(entry->submenu.caption, row, 0, 0);
				break;

			case kMenuItem_Cycler:
				MakeTextAtRowCol(entry->cycler.caption, row, 0, 0);
				RemapRedToGray(row);
				if (entry->cycler.numChoices > 0)
				{
					Byte index = *entry->cycler.valuePtr;
					const char* choiceCaption = NULL;
					if (index < MAX_CHOICES)
						choiceCaption = entry->cycler.choices[index];
					else
						choiceCaption = "???";
					MakeTextAtRowCol(choiceCaption, row, 1, textObjectFlags);
				}
				break;

			case kMenuItem_KeyBinding:
				MakeTextAtRowCol(kInputNeedCaptions[entry->kb], row, 0, 0);
				RemapRedToGray(row);
				for (int j = 0; j < KEYBINDING_MAX_KEYS; j++)
				{
					MakeTextAtRowCol(GetKeyBindingName(row, j), row, j + 1, textObjectFlags);
				}
				break;

			case kMenuItem_PadBinding:
				MakeTextAtRowCol(kInputNeedCaptions[entry->kb], row, 0, 0);
				RemapRedToGray(row);
				for (int j = 0; j < KEYBINDING_MAX_GAMEPAD_BUTTONS; j++)
				{
					MakeTextAtRowCol(GetPadBindingName(row, j), row, j + 1, textObjectFlags);
				}
				break;

			default:
				DoFatalAlert("Unsupported menu item type");
				break;
		}

		if (entry->type == kMenuItem_Separator)
			y += kRowHeight / 2;
		else
			y += kRowHeight;

		gNumMenuEntries++;
		GAME_ASSERT(gNumMenuEntries < MAX_ENTRIES_PER_MENU);
	}

#if EXPOSE_DITHERING
	if (gMenu == gVideoMenu)
		DrawDitheringPattern();
#endif

	ForceUpdateBackground();
	DumpBackground();											// dump to playfield
}

static void DrawDitheringPattern(void)
{
	int row = FindRowControlling(&gGamePrefs.filterDithering);
	GAME_ASSERT(row >= 0);

	uint8_t* ditheringPatternPlot = (uint8_t*) *gBackgroundHandle;
	ditheringPatternPlot += (gScreenYOffset + gMenuRowYs[row] - kRowHeight/3) * OFFSCREEN_WIDTH;
	ditheringPatternPlot += gScreenXOffset + kColumnX[1];
	for (int y = 0; y <= 2*kRowHeight/3; y++)
	{
		for (int x = y % 2; x < 20; x += 2)
		{
			ditheringPatternPlot[x] = 215;
		}
		ditheringPatternPlot += OFFSCREEN_WIDTH;
	}
}

/****************************/
/*    PUBLIC                */
/****************************/
#pragma mark - Public

void DoSettingsScreen(void)
{
					/* INITIAL LOADING */

	FadeOutGameCLUT();
	InitObjectManager();
	LoadShapeTable(":shapes:highscore.shapes", GROUP_WIN);
	LoadShapeTable(":shapes:fairy2.shapes", GROUP_AREA_SPECIFIC);		// cursor
	LoadBackground(":images:winbw.tga");								// just to load the palette

						/* LETS DO IT */

	gExitMenu		= false;
	gMenuRow		= 0;
	gMenu			= nil;			// LayOutMenu acts slightly different depending on current menu, so clear out gMenu
	gNumMenuEntries	= 0;

	LayOutMenu(gRootMenu);

	// Make cursor
	ObjNode* cursor = MakeNewShape(GROUP_AREA_SPECIFIC, ObjType_Spider, 1,
								   64, gMenuRowYs[gMenuRow], FARTHEST_Z, MoveCursor, 0);
	cursor->AnimSpeed = 0x080;

	do
	{
		RegulateSpeed2(1);									// @ 60fps
		EraseObjects();
		MoveObjects();
		SortObjectsByY();

		DrawObjects();
		DumpUpdateRegions();

		if (gScreenBlankedFlag)
			FadeInGameCLUT();

		ReadKeyboard();

		if (gAwaitingKeyPressForRebind)
			OnAwaitingKeyPress();
		else
			NavigateMenu();

		DoSoundMaintenance(true);							// (must be after readkeyboard)

	} while (!gExitMenu);

	FadeOutGameCLUT();

	SavePrefs();

	ZapShapeTable(GROUP_WIN);
	ZapShapeTable(GROUP_AREA_SPECIFIC);
}

void ApplyPrefs(void)
{
	OnChangePlayfieldSize();
	SetFullscreenMode(true);
	OnChangeIntegerScaling();
	SetPaletteColorCorrection();
}

