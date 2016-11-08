#include "tgfx.h"

typedef struct scaler scaler;

struct scaler {
	int size;
	int len;
	float factor;

	int pos;
	int idx;
	float value;

	scaler *prev;
	scaler *next;

	void (*func)(void *dst, void *src, scaler *sd);
};

void scale_data(void *dst, void *src, scaler *sd) {
	if (!dst || !src || !sd) return;

	float pos = 0.0, factor = sd->factor;
	int start = 0, end = sd->size, dir = 1;
	if (factor < 0.0) {
		start = sd->size-1;
		end = -1;
		dir = -1;
		factor = -factor;
	}

	int i;
	for (i = start; i != end; i += dir) {
		float span_left = factor;
		while (span_left > 0.0) {
			int p = (int)pos;
			float block = 1.0 - (pos - (float)p);
			float amount = span_left;
			if (block < span_left) {
				amount = block;
				pos += amount;
				span_left -= amount;
			}
			else {
				pos += span_left;
				span_left = 0.0;
			}
			sd->pos = p;
			sd->idx = i;
			sd->value = amount;

			scale_data(dst, src, sd->next);
			if (sd->func) sd->func(dst, src, sd);
		}
	}
}

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
/*
int trim_to16(tcolour *c) {
	int r = ((int)c->r * (int)c->a) / 0xff;
	int g = ((int)c->g * (int)c->a) / 0xff;
	int b = ((int)c->b * (int)c->a) / 0xff;

	int lum = (r+g+b) / 3;
	
	
*/
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

void trim_fillsprite(tsprite *spr, tpixel *p) {
	if (!spr) return;

	tpixel pixel = {0};
	if (!p) trim_createpixel(&pixel, NULL, NULL, ' ');
	else memcpy(&pixel, p, sizeof(tpixel));

	int i;
	for (i = 0; i < spr->w * spr->h; i++) memcpy(&spr->pix[i], &pixel, sizeof(tpixel));
}

tsprite *trim_createsprite(int w, int h, int x, int y, int mode) {
	if (w < 1 || h < 1 || x < 0 || y < 0 || mode < 0) return NULL;

	tsprite *spr = calloc(1, sizeof(tsprite));
	spr->w = w;
	spr->h = h;
	spr->x = x;
	spr->y = y;
	spr->mode = mode;
	spr->pix = calloc(w * h, sizeof(tpixel));

	tpixel p = {0};
	trim_createpixel(&p, NULL, NULL, 0);
	trim_fillsprite(spr, &p);
	return spr;
}
/*
FILE *debug = NULL;

void debug_colour(tcolour *c, char *name) {
	if (!debug) return;
	fprintf(debug,
		"%s:\n"
		"  r = %d\n"
		"  g = %d\n"
		"  b = %d\n"
		"  a = %d\n",
		name ? name : "colour", c->r, c->g, c->b, c->a);
}

void debug_pixel(tpixel *p) {
	if (!debug) return;
	debug_colour(&p->bg, "bg");
	debug_colour(&p->fg, "fg");
	fprintf(debug, "ch: '%c'\n\n", p->ch);
}
*/
void trim_blendcolour(tcolour *dst, tcolour *src) {
	if (!dst) return;

	u8 *dst_p = (u8*)dst;
	u8 src_p[4] = {0};
	//float dst_a;
	float src_a;
	if (src) {
		memcpy(&src_p, src, sizeof(tcolour));
		//dst_a = (float)dst->a / 255.0;
		src_a = (float)src->a / 255.0;
	}
	else {
		//dst_a = 1.0;
		src_a = (float)(255 - dst->a) / 255.0;
	}

	int i;
	for (i = 0; i < 3; i++) {
		float dst_chn = (float)(dst_p[i]) / 255.0;
		float src_chn = (float)(src_p[i]) / 255.0;

		dst_chn += src_chn * src_a;
		dst_p[i] = (u8)(dst_chn * 255.0);
	}
}

FILE *debug = NULL;

void trim_applysprite(tsprite *dst, tsprite *src) {
	if (!dst || !src) return;
	if (src->x + src->w <= 0 || src->x >= dst->w) return;
	if (src->y + src->h <= 0 || src->y >= dst->h) return;

	int dx = src->x, dy = src->y, sx = 0, sy = 0;
	if (src->x < 0) {
		dx = 0;
		sx = -src->x;
	}
	if (src->y < 0) {
		dy = 0;
		sy = -src->y;
	}

	int w = src->w - sx;
	if (w > dst->w - dx) w = dst->w - dx;

	int h = src->h - sy;
	if (h > dst->h - dy) h = dst->h - dy;

	if (!debug) debug = fopen("as_debug.txt", "w");
	fprintf(debug, "dx: %d, dy: %d, sx: %d, sy: %d, w: %d, h: %d\n", dx, dy, sx, sy, w, h);

	int i, j;
	for (i = 0; i < h; i++) {
		for (j = 0; j < w; j++) {
			tpixel *dp = &dst->pix[(dy+i) * dst->w + dx+j];
			tpixel *sp = &src->pix[(sy+i) * w + sx+j];

			trim_blendcolour(&dp->bg, &sp->bg);
			trim_blendcolour(&dp->fg, &sp->fg);
			if (sp->ch) dp->ch = sp->ch;
		}
	}
}

void trim_printsprite(tsprite *spr, char *str, int x, int y, int lr) {
	if (!spr || !str) return;

	char *p = str;
	if (!lr) {
		if (x < 0) {
			p += -x;
			x = 0;
		}
		if (p >= str+strlen(str) || x >= spr->w || y < 0 || y >= spr->h) return;
	}
	else {
		if (x < 0) {
			x += spr->w;
			y--;
		}
		if (x >= spr->w) {
			x -= spr->w;
			y++;
		}
		if (y < 0) {
			p += (-y * spr->w) + x;
			x = y = 0;
		}
		if (p >= str+strlen(str) || y >= spr->h) return;
	}

	int i, j;
	for (i = y; i < spr->h; i++) {
		for (j = x; j < spr->w && *p; j++, p++) {
			if (*p == '\n') break;
			spr->pix[i*spr->w+j].ch = *p;
		}
		if (*p != '\n' && (!lr || *p == 0)) break;
		x = 0;
	}
	return;
}

typedef struct {
	float r, g, b, a;
} tclf;

float set_range(float in, float low, float high) {
	if (in < low) return low;
	if (in > high) return high;
	return in;
}

void resize_pixel(void *dst, void *src, scaler *sd) {
	tclf *d = (tclf*)dst;
	tcolour *s = (tcolour*)src;

	// sd contains info about x whereas sd->prev contains info about y
	float area = sd->value * sd->prev->value;
	if (area <= 0.0001) return;

	int s_idx = sd->prev->idx * sd->size + sd->idx;

	int d_width = (sd->size * sd->factor) + 0.5; // make sure it's rounded correctly
	if (d_width < 0) d_width = -d_width;
	int d_idx = sd->prev->pos * d_width + sd->pos;

	d[d_idx].r = set_range(d[d_idx].r + (float)s[s_idx].r * area, 0.0, 255.0);
	d[d_idx].g = set_range(d[d_idx].g + (float)s[s_idx].g * area, 0.0, 255.0);
	d[d_idx].b = set_range(d[d_idx].b + (float)s[s_idx].b * area, 0.0, 255.0);
	d[d_idx].a = set_range(d[d_idx].a + (float)s[s_idx].a * area, 0.0, 255.0);
}

void trim_scaletexture(ttexture *dst, ttexture *src, int w, int h) {
	if (!dst || !src || !src->img || src->w < 1 || src->h < 1) return;

	if (dst->img) free(dst->img);

	if (w == 0.0 || h == 0.0) {
		dst->img = NULL;
		return;
	}

	float x_sc = (float)w / (float)src->w;
	if (w < 0) w = -w;
	dst->w = w;

	float y_sc = (float)h / (float)src->h;
	if (h < 0) h = -h;
	dst->h = h;

	dst->img = calloc(dst->w * dst->h, sizeof(tcolour));

	scaler w_rs = {0};
	scaler h_rs = {0};

	w_rs.size = src->w;
	w_rs.factor = x_sc;
	w_rs.func = resize_pixel;
	w_rs.prev = &h_rs;

	h_rs.size = src->h;
	h_rs.factor = y_sc;
	h_rs.next = &w_rs;

	tclf *rs = calloc(dst->w * dst->h, sizeof(tclf));
	scale_data(rs, src->img, &h_rs);

	int i;
	for (i = 0; i < dst->w * dst->h; i++) {
		dst->img[i].r = (u8)rs[i].r;
		dst->img[i].g = (u8)rs[i].g;
		dst->img[i].b = (u8)rs[i].b;
		dst->img[i].a = (u8)rs[i].a;
	}
	free(rs);
}

void trim_rendertexture(tsprite *spr, ttexture *tex, int x, int y, int w, int h) {
	if (!spr || !tex) return;

	ttexture sc_tex = {0};
	trim_scaletexture(&sc_tex, tex, w, h);

	tsprite new_spr = {0}; // names are hard

	new_spr.x = x;
	new_spr.y = y;
	new_spr.w = sc_tex.w;
	new_spr.h = sc_tex.h;
	new_spr.mode = TRIM_RGB;
	new_spr.pix = calloc(sc_tex.w * sc_tex.h, sizeof(tpixel));

	int i;
	for (i = 0; i < sc_tex.w * sc_tex.h; i++) {
		memcpy(&new_spr.pix[i].bg, &sc_tex.img[i], sizeof(tcolour));
		memset(&new_spr.pix[i].fg, 0, sizeof(tcolour));
		new_spr.pix[i].ch = ' ';
	}
	free(sc_tex.img);

	if (!spr->pix) {
		memcpy(spr, &new_spr, sizeof(tsprite));
		return;
	}

	trim_applysprite(spr, &new_spr);
	free(new_spr.pix);
}

void trim_drawsprite(tsprite *s) {
	if (!s) return;
	int r, g, b;
	int i, j, p = 0;
	for (i = 0; i < s->h; i++) {
		printf("\x1b[%d;%dH", s->y+i+1, s->x+1);

		for (j = 0; j < s->w; j++, p++) {
			trim_blendcolour(&s->pix[p].bg, NULL);
			trim_blendcolour(&s->pix[p].fg, NULL);

			if (s->mode == TRIM_256) {
				int bg = trim_to256(&s->pix[p].bg);
				int fg = trim_to256(&s->pix[p].fg);
				printf("\x1b[48;5;%dm", bg);
				//fflush(stdout);
				printf("\x1b[38;5;%dm", fg);
			}
			else if (s->mode == TRIM_RGB) {
				printf("\x1b[48;2;%d;%d;%dm", s->pix[p].bg.r, s->pix[p].bg.g, s->pix[p].bg.b);
				fflush(stdout);
				printf("\x1b[38;2;%d;%d;%dm", s->pix[p].fg.r, s->pix[p].fg.g, s->pix[p].fg.b);
			}
			//fflush(stdout);

			char c = s->pix[p].ch;
			putchar((c > 0x1f && c < 0x7f) ? c : ' ');
			fflush(stdout);
		}
		printf("\x1b[0m");
		fflush(stdout);
	}
}

void trim_closesprite(tsprite *s) {
	if (!s) return;
	if (s->pix) free(s->pix);
	memset(s, 0, sizeof(tsprite));
}

tsprite *trim_initvideo(int win_w, int win_h, int sc_w, int sc_h, int mode) {
	int x = (win_w - sc_w) / 2;
	int y = (win_h - sc_h) / 2;
	if (x < 0) x = 0;
	if (y < 0) y = 0;

	printf("\x1b[?25l");
	system("clear");
	return trim_createsprite(sc_w, sc_h, x, y, mode);
}

void trim_closevideo(tsprite *s) {
	printf("\x1b[0m");
	int i, j;
	for (i = 0; i < s->h; i++) {
		printf("\x1b[%d;%dH", s->y+1 + i, s->x+1);
		for (j = 0; j < s->w; j++) putchar(' ');
	}
	printf("\x1b[?25h\n");
	trim_closesprite(s);
	free(s);
}
