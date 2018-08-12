#ifndef TRIM_H
#define TRIM_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
  #include <windows.h>
#else
  #include <unistd.h>
  #include <termios.h>
  #include <fcntl.h>
  #include <sys/types.h>
  #include <sys/stat.h>
  #include <sys/ioctl.h>
  #include <signal.h>
  #ifdef __linux__
    #include <linux/kd.h>
    #include <linux/input.h>
  #endif
#endif

typedef struct {
	unsigned char r, g, b, a;
} TRIM_Color;

typedef struct {
	char *ch;
	TRIM_Color *bg;
	TRIM_Color *fg;
	int w, h;
	int x, y;
	int colour_mode;
	int ch_blend;
} TRIM_Sprite;

typedef struct {
	TRIM_Color *img;
	int w, h;
} TRIM_Texture;

/*
typedef struct {
	float tx, ty; // Coordinates of a point as mapped on a sprite
	float sx, sy; // Coordinates of a point as mapped to the screen
} tpoint;

typedef struct {
	TRIM_Texture tex;
	tpoint point[4];
	int n_points;
} tpolygon;
*/

// input
void TRIM_InitKB(int kb_mode);

void TRIM_PollKB(void);
int TRIM_GetKey(void);

int TRIM_KeyDown(int key);
int TRIM_KeyHeld(int key);
int TRIM_KeyUp(int key);

void TRIM_CloseKB(void);

// video
int TRIM_InitVideo(int mode);
void TRIM_CloseVideo(int keep_screen);

void TRIM_GetConsoleSize(int *w, int *h);
int TRIM_SetConsoleSize(int w, int h);

void TRIM_DrawScreen(void);
void TRIM_ClearScreen(void);
void TRIM_ResizeScreen(void);
TRIM_Sprite *TRIM_GetScreen(void);

// gfx

#ifndef _WIN32_

int TRIM_to256(TRIM_Color *c);
int TRIM_16to256(int x);

#endif

int TRIM_to16(TRIM_Color *c);

void TRIM_BlendColor(TRIM_Color *dst, TRIM_Color *src);

void TRIM_ApplySprite(TRIM_Sprite *dst, TRIM_Sprite *src);
void TRIM_ResizeSprite(TRIM_Sprite *s, int w, int h);
void TRIM_CloseSprite(TRIM_Sprite *s);

void TRIM_ApplyTexture(TRIM_Texture *tex, int x, int y, int w, int h);

//void TRIM_renderpolygon(TRIM_Texture *tex, tpolygon *poly);
void TRIM_ScaleTexture(TRIM_Texture *dst, TRIM_Texture *src, int w, int h);

int TRIM_OpenBMP(TRIM_Texture *tex, char *name);


// Colour modes. 0 = full colour mode, 1 = 256 colours, 2 = 16 colours
#define TGFX_RGB 0
#define TGFX_256 1
#define TGFX_16  2

// Character blending modes. 0 = all chars opaque, 1 = all chars but space are opaque, 2 = all chars transparent
#define TGFX_CH_OPQ 0
#define TGFX_CH_DEF 1
#define TGFX_CH_TRN 2

/*
   Keyboard input modes.
   0 = Default keyboard mode, which allows key repeating and other such features, however key release is only supported on Windows.
   1 = Raw keyboard mode, where the only types of input are key press and key release. Mac OS X and some other Unix systems are not supported.
   In short, Default mode is good for applications, and Raw mode is good for games.
*/
#define TRIM_DEFKB 0
#define TRIM_RAWKB 1

// Custom index for every US standard keyboard key
#define TKEY_GRAVE    0
#define TKEY_1        1
#define TKEY_2        2
#define TKEY_3        3
#define TKEY_4        4
#define TKEY_5        5
#define TKEY_6        6
#define TKEY_7        7
#define TKEY_8        8
#define TKEY_9        9
#define TKEY_0       10
#define TKEY_MINUS   11
#define TKEY_EQUAL   12
#define TKEY_BKSP    13
#define TKEY_TAB     14
#define TKEY_Q       15
#define TKEY_W       16
#define TKEY_E       17
#define TKEY_R       18
#define TKEY_T       19
#define TKEY_Y       20
#define TKEY_U       21
#define TKEY_I       22
#define TKEY_O       23
#define TKEY_P       24
#define TKEY_LBRACE  25
#define TKEY_RBRACE  26
#define TKEY_BKSL    27
#define TKEY_CAPS    28
#define TKEY_A       29
#define TKEY_S       30
#define TKEY_D       31
#define TKEY_F       32
#define TKEY_G       33
#define TKEY_H       34
#define TKEY_J       35
#define TKEY_K       36
#define TKEY_L       37
#define TKEY_SMCOL   38
#define TKEY_APOS    39
#define TKEY_ENTER   40
#define TKEY_LSHIFT  41
#define TKEY_Z       42
#define TKEY_X       43
#define TKEY_C       44
#define TKEY_V       45
#define TKEY_B       46
#define TKEY_N       47
#define TKEY_M       48
#define TKEY_COMMA   49
#define TKEY_DOT     50
#define TKEY_FWSL    51
#define TKEY_RSHIFT  52
#define TKEY_LCTRL   53
#define TKEY_SYS1    54
#define TKEY_LALT    55
#define TKEY_SPACE   56
#define TKEY_RALT    57
#define TKEY_SYS2    58
#define TKEY_MENU    59
#define TKEY_RCTRL   60

#define TKEY_ESC     61
#define TKEY_F1      62
#define TKEY_F2      63
#define TKEY_F3      64
#define TKEY_F4      65
#define TKEY_F5      66
#define TKEY_F6      67
#define TKEY_F7      68
#define TKEY_F8      69
#define TKEY_F9      70
#define TKEY_F10     71
#define TKEY_F11     72
#define TKEY_F12     73
#define TKEY_PRINT   74
#define TKEY_SCROLL  75
#define TKEY_PAUSE   76
#define TKEY_INSERT  77
#define TKEY_HOME    78
#define TKEY_PGUP    79
#define TKEY_DEL     80
#define TKEY_END     81
#define TKEY_PGDOWN  82
#define TKEY_UP      83
#define TKEY_LEFT    84
#define TKEY_DOWN    85
#define TKEY_RIGHT   86

#define TKEY_NLOCK   87
#define TKEY_KPFWSL  88
#define TKEY_KPAST   89
#define TKEY_KPMIN   90
#define TKEY_KP7     91
#define TKEY_KP8     92
#define TKEY_KP9     93
#define TKEY_KPPLUS  94
#define TKEY_KP4     95
#define TKEY_KP5     96
#define TKEY_KP6     97
#define TKEY_KP1     98
#define TKEY_KP2     99
#define TKEY_KP3    100
#define TKEY_KPENT  101
#define TKEY_KP0    102
#define TKEY_KPDOT  103

#define TRIM_NKEYS  104

#endif
