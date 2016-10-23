#include "tgfx.h"

void get_rgb(int c, int *r, int *g, int *b) {
	if (r) *r = (c >> 16) & 0xff;
	if (g) *g = (c >> 8) & 0xff;
	if (b) *b = c & 0xff;
}

int set_rgb(int r, int g, int b) {
	if (r < 0 || r > 255 || g < 0 || g > 255 || b < 0 || b > 255) return 0;
	return (r << 16) | (b << 8) | g;
}

int trim_to256(tcolour *c) {
	int r = ((int)c->r * (int)c->a) / 0xff;
	int g = ((int)c->g * (int)c->a) / 0xff;
	int b = ((int)c->b * (int)c->a) / 0xff;

	int lum = (r+g+b) / 3, range = 0x16;
	int i, pixel[] = {r, g, b};
	for (i = 0; i < 3; i++) {
		if (pixel[i] < lum-range || pixel[i] > lum+range) {
			r = (r * 6) / 256;
			g = (g * 6) / 256;
			b = (b * 6) / 256;
			return (r*36 + g*6 + b) + 0x10;
		}
	}
	return ((lum * 0x18) / 256) + 0xe8;
}

void trim_createpixel(tpixel *p, tcolour *bg, tcolour *fg, char ch) {
	if (!p) return;

	tcolour b = {0, 0, 0, 0xff};
	tcolour f = {0xff, 0xff, 0xff, 0xff};
	if (bg) memcpy(&b, bg, sizeof(tcolour));
	if (fg) memcpy(&f, fg, sizeof(tcolour));

	memcpy(&p->bg, &b, sizeof(tcolour));
	memcpy(&p->fg, &f, sizeof(tcolour));
	p->ch = ch;
}

void trim_filltexture(ttexture *t, tpixel *p) {
	if (!t || !p) return;
	int i;
	for (i = 0; i < t->w * t->h; i++) memcpy(&t->pix[i], p, sizeof(tpixel));
}

ttexture *trim_createtexture(int w, int h, int x, int y, int mode) {
	if (w < 1 || h < 1 || x < 0 || y < 0 || mode < 0) return NULL;

	ttexture *tex = calloc(1, sizeof(ttexture));
	tex->w = w;
	tex->h = h;
	tex->x = x;
	tex->y = y;
	tex->mode = mode;

	tex->pix = calloc(w * h, sizeof(int));
	trim_filltexture(tex, NULL, NULL, 0);
	return tex;
}

void trim_blendcolour(tcolour *dst, tcolour *src) {
	if (!dst || !src) return;

	u8 *dst_p[] = {&dst->r, &dst->g, &dst->b};
	u8 *src_p[] = {&src->r, &src->g, &src->b};
	float dst_a = (float)dst->a / 255.0;
	float src_a = (float)src->a / 255.0;

	int i;
	for (i = 0; i < 3; i++) {
		float dst_chn = (float)(*dst_p[i]) / 255.0;
		float src_chn = (float)(*src_p[i]) / 255.0;

		

void trim_applytexture(ttexture *s, ttexture *tex) {
	if (!s || !tex) return;
	if (tex->x + tex->w <= 0 || tex->x >= s->w) return;
	if (tex->y + tex->h <= 0 || tex->y >= s->h) return;

	int ox = 0, oy = 0;
	if (tex->x < 0) ox = -tex->x;
	if (tex->y < 0) oy = -tex->y;

	int lx = tex->w, ly = tex->h;
	if (tex->x + tex->w > s->w) lx = s->w - tex->x - ox;
	if (tex->y + tex->h > s->h) ly = s->h - tex->y - oy;

	int i, j;
	for (i = oy; i < ly; i++) {
		for (j = ox; j < lx; j++) {
			tpixel *dst = &s->pix[(i-oy) * lx + (j-ox)];
			tpixel *src = &tex->pix[i*lx + j];
			
			trim_blendcolour(&dst->bg, &src->bg);
			trim_blendcolour(&dst->fg, &src->fg);
			dst->ch = src->ch;
		}
	}
}

void trim_drawtexture(ttexture *s) {
	if (!s) return;
	int r, g, b;
	int i, j, p = 0;
	for (i = 0; i < s->h; i++) {
		printf("\x1b[%d;%dH", s->y+i+1, s->x+1);
		for (j = 0; j < s->w; j++, p++) {
			
			if (s->mode == MODE_256) {
				int bg = trim_to_256(&s->pix[p].bg);
				int fg = trim_to_256(&s->pix[p].fg);
				printf("\x1b[48;5;%dm\x1b[38;5;%dm", bg, fg);
			}
			else if (s->mode == MODE_RGB) {
				printf("\x1b[48;2;%d;%d;%dm\x1b[38;2;%d;%d;%dm",
					s->pix[p].bg.r, s->pix[p].bg.g, s->pix[p].bg.b,
					s->pix[p].fg.r, s->pix[p].fg.g, s->pix[p].fg.b);
			}
			char c = s->pix[p].ch;
			putchar((c > 0x1f && c < 0x7f) ? c : ' ');
		}
	}
	printf("\x1b[0m");
	fflush(stdout);
}

void trim_closetexture(ttexture *tex) {
	if (!tex) return;
	if (tex->pix) free(tex->pix);
	memset(tex, 0, sizeof(ttexture));
}