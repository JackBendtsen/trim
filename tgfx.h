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

ttexture *trim_createtexture(int w, int h, int x, int y, int mode);
void trim_filltexture(ttexture *tex, int colour);
void trim_applytexture(ttexture *s, ttexture *tex);
void trim_drawtexture(ttexture *s);
void trim_closetexture(ttexture *tex);

#endif