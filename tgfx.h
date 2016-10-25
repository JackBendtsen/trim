#ifndef TGFX_H
#define TGFX_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MODE_256 0
#define MODE_RGB 1

typedef unsigned char u8;
typedef unsigned int u32;

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
} ttexture;

ttexture *trim_initvideo(int win_w, int win_h, int sc_w, int sc_h, int mode);
void trim_closevideo(ttexture *s);

void trim_blendcolour(tcolour *dst, tcolour *src);

void trim_createpixel(tpixel *p, tcolour *bg, tcolour *fg, char ch);
ttexture *trim_createtexture(int w, int h, int x, int y, int mode);

void trim_filltexture(ttexture *tex, tpixel *p);
void trim_applytexture(ttexture *s, ttexture *tex);
void trim_printtexture(ttexture *tex, char *str, int x, int y, int lr);
void trim_drawtexture(ttexture *s);
void trim_closetexture(ttexture *tex);

#endif
