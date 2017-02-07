#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32_
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
  struct termios _trim_tty_old;
  int _trim_evfd;
  fd_set _trim_fdset;
#endif

#define TGFX_RGB 0
#define TGFX_256 1
#define TGFX_16  2

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
#define TKEY_SCLOCK  75
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

/*
#define TKEY_UP    0x1b5b41
#define TKEY_DOWN  0x1b5b42
#define TKEY_RIGHT 0x1b5b43
#define TKEY_LEFT  0x1b5b44
*/

#define TRIM_RAWKB 1
#define TRIM_DEFKB 0

typedef unsigned char u8;
typedef unsigned int u32;

typedef struct {
	int idx;
	int state;
} tkey;

typedef struct {
	u8 r, g, b, a;
} tcolour;

typedef struct {
	tcolour bg;
	tcolour fg;
	char ch;
} tpixel;

typedef struct {
	tpixel *pix;
	int w, h;
	int x, y;
	int mode;
} tsprite;

typedef struct {
	tcolour *img;
	int w, h;
} ttexture;

/*
typedef struct {
	float tx, ty; // Coordinates of a point as mapped on a sprite
	float sx, sy; // Coordinates of a point as mapped to the screen
} tpoint;

typedef struct {
	ttexture tex;
	tpoint point[4];
	int n_points;
} tpolygon;
*/

static const tcolour _trim_16cp[] = {
	{  0,   0,   0, 255}, {  0,   0, 128, 255},
	{  0, 128,   0, 255}, {  0, 128, 128, 255},
	{128,   0,   0, 255}, {128,   0, 128, 255},
	{128, 128,   0, 255}, {192, 192, 192, 255},
	{128, 128, 128, 255}, {  0,   0, 255, 255},
	{  0, 230,   0, 255}, {  0, 255, 255, 255},
	{255,   0,   0, 255}, {255,   0, 255, 255},
	{255, 255,   0, 255}, {255, 255, 255, 255}
};

int trim_kb_mode;
int trim_keycode[TRIM_NKEYS];

tkey *trim_old_kbst;
tkey *trim_cur_kbst;
int trim_old_kbsize;
int trim_cur_kbsize;

tsprite *trim_screen;
int trim_old_w;
int trim_old_h;

void trim_initkb(int kb_mode);

void trim_pollkb(void);
int trim_getkey(void);

int trim_keydown(int key);
int trim_keyheld(int key);
int trim_keyup(int key);

void trim_closekb(void);

void trim_initvideo(int mode);
void trim_closevideo();

int trim_openbmp(ttexture *tex, char *name);

void trim_blendcolour(tcolour *dst, tcolour *src);

void trim_createpixel(tpixel *p, tcolour *bg, tcolour *fg, char ch);
tsprite *trim_createsprite(int w, int h, int x, int y, int mode);

void trim_fillsprite(tsprite *spr, tpixel *p);
void trim_applysprite(tsprite *dst, tsprite *src);
void trim_printsprite(tsprite *spr, char *str, int x, int y, int lr);
void trim_resizesprite(tsprite *s, int w, int h);

//void trim_renderpolygon(ttexture *tex, tpolygon *poly);
void trim_scaletexture(ttexture *dst, ttexture *src, int w, int h);
void trim_rendertexture(tsprite *spr, ttexture *tex, int x, int y, int w, int h);

void trim_drawsprite(tsprite *s);
void trim_closesprite(tsprite *s);

#endif
