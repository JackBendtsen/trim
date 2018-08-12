#include "../include/trim.h"

typedef unsigned char u8;

// Colour palette for the standard Windows command prompt
// Used for mapping internal colours to console output in 16-colour mode for all backends
const TRIM_Color _TRIM_16cp[] = {
	{0x00, 0x00, 0x00, 0xff}, {0x00, 0x00, 0x80, 0xff},
	{0x00, 0x80, 0x00, 0xff}, {0x00, 0x80, 0x80, 0xff},
	{0x80, 0x00, 0x00, 0xff}, {0x80, 0x00, 0x80, 0xff},
	{0x80, 0x80, 0x00, 0xff}, {0xc0, 0xc0, 0xc0, 0xff},
	{0x80, 0x80, 0x80, 0xff}, {0x00, 0x00, 0xff, 0xff},
	{0x00, 0xe6, 0x00, 0xff}, {0x00, 0xff, 0xff, 0xff},
	{0xff, 0x00, 0x00, 0xff}, {0xff, 0xff, 0xff, 0xff},
	{0xff, 0xff, 0x00, 0xff}, {0xff, 0xff, 0xff, 0xff}
};

#ifndef _WIN32

// Convert sRGB to 8-bit ANSI colour index
// Details: https://en.wikipedia.org/wiki/ANSI_escape_code#8-bit
int TRIM_to256(TRIM_Color *c) {
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

int TRIM_16to256(int x) {
	if (x < 0 || x > 15) return 0;

	// Hand-picked set of ANSI colour indices that most closely resemble the Windows console colour palette
	int p[] = {
		0x10, 0x12, 0x1c, 0x1e,
		0x58, 0x5a, 0x64, 0xfa,
		0xf4, 0x15, 0x2e, 0x33,
		0xc4, 0xc9, 0xe2, 0xe7
	};
	return p[x];
}

#endif

// Match sRGB to the nearest Windows console colour
int TRIM_to16(TRIM_Color *c) {
	int r = ((int)c->r * (int)c->a) / 0xff;
	int g = ((int)c->g * (int)c->a) / 0xff;
	int b = ((int)c->b * (int)c->a) / 0xff;

	int i, idx, low = -1, delta;
	for (i = 0; i < 16; i++) {
		int rd = r - _TRIM_16cp[i].r;
		int gd = g - _TRIM_16cp[i].g;
		int bd = b - _TRIM_16cp[i].b;
		if (rd < 0) rd = -rd;
		if (gd < 0) gd = -gd;
		if (bd < 0) bd = -bd;

		delta = rd + gd + bd;
		if (low < 0 || delta < low) {
			low = delta;
			idx = i;
		}
	}

	return idx;
}

void TRIM_BlendColor(TRIM_Color *dst, TRIM_Color *src) {
	if (!dst)
		return;

	if (!src) {
		dst->a = 0xff;
		return;
	}

	u8 *dst_pixel = (u8*)dst;
	u8 *src_pixel = (u8*)src;

	float dst_alpha = (float)dst->a / 255.0;
	float src_alpha = (float)src->a / 255.0;

	int i;
	for (i = 0; i < 3; i++) {
		float dst_chn = (float)(dst_pixel[i]) / 255.0;
		float src_chn = (float)(src_pixel[i]) / 255.0;

		dst_chn += src_chn * src_alpha;

		if (dst_chn < 0.0) dst_chn = 0.0;
		if (dst_chn > 1.0) dst_chn = 1.0;

		dst_pixel[i] = (u8)(dst_chn * 255.0);
	}

	dst_alpha += (1.0 - dst_alpha) * src_alpha;
	dst->a = (u8)(dst_alpha * 255.0);
}

void TRIM_ApplySprite(TRIM_Sprite *dst, TRIM_Sprite *src) {
	if (!dst)
		dst = TRIM_GetScreen();

	if (!dst || !src || 
	    src->x + src->w <= 0 || src->x >= dst->w ||
	    src->y + src->h <= 0 || src->y >= dst->h)
		return;

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

	int i, j;
	for (i = 0; i < h; i++) {
		for (j = 0; j < w; j++) {
			int dst_idx = (dy+i) * dst->w + dx+j;
			int src_idx = (sy+i) * src->w + sx+j;

			if (src->bg && dst->bg)
				TRIM_BlendColor(&dst->bg[dst_idx], &src->bg[src_idx]);

			if (src->fg && src->bg)
				TRIM_BlendColor(&dst->fg[dst_idx], &src->fg[src_idx]);

			if (src->ch && dst->ch)
				dst->ch[dst_idx] = src->ch[src_idx];
		}
	}
}

// Changes the size of a sprite, without scaling.
// Any gaps created in resizing (ie. new width > old width or new height > old height)
//  are filled in with the default colour.
void TRIM_ResizeSprite(TRIM_Sprite *spr, int w, int h) {
	if (!spr || w < 1 || h < 1 || (spr->w == w && spr->h == h))
		return;

	spr->bg = realloc(spr->bg, w * h * sizeof(TRIM_Color));
	spr->fg = realloc(spr->fg, w * h * sizeof(TRIM_Color));
	spr->ch = realloc(spr->ch, w * h);

	TRIM_Color def_bg = {0, 0, 0, 0xff};
	TRIM_Color def_fg = {0xff, 0xff, 0xff, 0xff};

	int i, j;
	if (w > spr->w) {
		for (i = h-1; i >= 0; i--) {
			if (i) {
				memmove(spr->bg + i*w, spr->bg + i*spr->w, spr->w * sizeof(TRIM_Color));
				memmove(spr->fg + i*w, spr->fg + i*spr->w, spr->w * sizeof(TRIM_Color));
				memmove(spr->ch + i*w, spr->ch + i*spr->w, spr->w);
			}
			for (j = spr->w; j < w; j++) {
				spr->bg[i*w + j] = def_bg;
				spr->fg[i*w + j] = def_fg;
			}
			memset(&spr->ch[i*w + spr->w], ' ', w - spr->w);
		}
	}
	if (h > spr->h) {
		for (i = spr->h; i < h; i++) {
			for (j = 0; j < w; j++) {
				spr->bg[i*w + j] = def_bg;	
				spr->fg[i*w + j] = def_fg;
			}
		}
		memset(&spr->ch[w * spr->h], ' ', w * (h - spr->h));
	}

	spr->w = w;
	spr->h = h;
}

void TRIM_CloseSprite(TRIM_Sprite *s) {
	if (!s) return;
	if (s->fg) free(s->fg);
	if (s->bg) free(s->bg);
	if (s->ch) free(s->ch);
	memset(s, 0, sizeof(TRIM_Sprite));
}

void TRIM_ApplyTexture(TRIM_Texture *tex, int x, int y, int w, int h) {
	if (!tex) return;

	TRIM_Texture sc_tex = {0};
	TRIM_ScaleTexture(&sc_tex, tex, w, h);

	TRIM_Sprite new_spr = {0}; // names are hard

	new_spr.x = x;
	new_spr.y = y;
	new_spr.w = sc_tex.w;
	new_spr.h = sc_tex.h;
	new_spr.bg = sc_tex.img;
	//new_spr.pix = calloc(sc_tex.w * sc_tex.h, sizeof(TRIM_Pixel));
	// no char buffer needed

	//memcpy(new_spr.bg, sc_tex.img, sc_tex.w * sc_tex.h * sizeof(TRIM_Color));
	//memset(new_spr.fg, 0, sizeof(TRIM_Color));

	TRIM_ApplySprite(TRIM_GetScreen(), &new_spr);
	//free(new_spr.pix);
	free(sc_tex.img);
}

// Image resizing
// Essentially stretches each line of pixels over two dimensions
//  using a custom data scaling solution

typedef struct {
	float r, g, b, a;
} tclf;

// Doubly-linked list with a function pointer for good measure
struct scaler {
	int size; // pre-operation length
	int len; // post-operation length
	float factor; // scaling factor

	int pos;
	int idx;
	float value;

	struct scaler *prev;
	struct scaler *next;

	void (*func)(void *dst, void *src, struct scaler *sd);
};

void scale_data(void *dst, void *src, struct scaler *sd) {
	if (!dst || !src || !sd) return;

	float pos = 0.0, factor = sd->factor;
	int start = 0, end = sd->size, dir = 1;
	if (factor < 0.0) {
		start = sd->size-1;
		end = -1;
		dir = -1;
		factor = -factor;
	}

	// For each input pixel, calculate which proportions of it belong in which output pixels
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

inline float set_range(float in, float low, float high) {
	if (in < low) return low;
	if (in > high) return high;
	return in;
}

void resize_pixel(void *dst, void *src, struct scaler *sd) {
	tclf *d = (tclf*)dst;
	TRIM_Color *s = (TRIM_Color*)src;

	// sd contains info about x whereas sd->prev contains info about y
	double area = sd->value * sd->prev->value;

	int s_idx = sd->prev->idx * sd->size + sd->idx;

	// Add a half to make sure they're rounded correctly
	float w_round = (sd->factor < 0.0 ? -1.0 : 1.0) * 0.5;
	float h_round = (sd->prev->factor < 0.0 ? -1.0 : 1.0) * 0.5;
	int d_width = (sd->size * sd->factor) + w_round;
	int d_height = (sd->prev->size * sd->prev->factor) + h_round;

	if (d_width < 0) d_width = -d_width;
	if (d_height < 0) d_height = -d_height;

	if (sd->pos >= d_width) return;
	if (sd->prev->pos >= d_height) return;

	int d_idx = sd->prev->pos * d_width + sd->pos;

	d[d_idx].r = set_range(d[d_idx].r + (float)s[s_idx].r * area, 0.0, 255.0);
	d[d_idx].g = set_range(d[d_idx].g + (float)s[s_idx].g * area, 0.0, 255.0);
	d[d_idx].b = set_range(d[d_idx].b + (float)s[s_idx].b * area, 0.0, 255.0);
	d[d_idx].a = set_range(d[d_idx].a + (float)s[s_idx].a * area, 0.0, 255.0);
}

void TRIM_ScaleTexture(TRIM_Texture *dst, TRIM_Texture *src, int w, int h) {
	if (!dst || !src || !src->img || src->w < 1 || src->h < 1) return;

	if (dst->img) free(dst->img);

	if (w == 0.0 || h == 0.0) {
		dst->img = NULL;
		return;
	}

	double x_sc = (double)w / (double)src->w;
	if (w < 0) w = -w;
	dst->w = w;

	double y_sc = (double)h / (double)src->h;
	if (h < 0) h = -h;
	dst->h = h;

	dst->img = calloc(dst->w * dst->h, sizeof(TRIM_Color));

	struct scaler w_rs = {0}, h_rs = {0};

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

typedef struct {
	int file_sz;      // Size of file in bytes
	short un1, un2;   // Unused variables (0)
	int off;          // Byte offset of pixel data in file (54)
	int dib_sz;       // Size of the DIB header (40)
	int x, y;         // Dimensions of the BMP
	short ndp;        // Number of drawing planes (1)
	short bpp;        // Bits per pixel
	int comp;         // Compression used (0)
	int data_sz;      // Size of pixel data including padding
	int x_res, y_res; // Resolution of the image in pixels per metre (0)
	int nc;           // Number of colours (0)
	int nic;          // Number of important colours (0)
} bmp_t;
/*
Note: this struct does not include the BMP signature, as it is a 2-byte field by itself,
      which would incorrectly offset the entire struct's byte alignment
*/

int TRIM_OpenBMP(TRIM_Texture *tex, char *name) {
	if (!tex || !name) return -1;

	FILE *f = fopen(name, "rb");
	if (!f) {
		printf("Could not open \"%s\"\n", name);
		return -2;
	}

	fseek(f, 0, SEEK_END);
	int sz = ftell(f);
	rewind(f);
	if (sz < sizeof(bmp_t)) {
		printf("\"%s\" isn't big enough to be a BMP file (size: %d)\n", name, sz);
		return -3;
	}

	u8 *buf = malloc(sz);
	fread(buf, 1, sz, f);
	fclose(f);

	bmp_t hdr = {0};
	memcpy(&hdr, buf+2, sizeof(bmp_t));
	//print_bmp_t(&hdr);

	int err = 0;
	if (memcmp(buf, "BM", 2)) {
		printf("\"%s\" is not a BMP file\n", name);
		err = -4;
	}
	else if (hdr.x < 1 || hdr.y < 1) {
		printf("\"%s\" has invalid dimensions (%dx%d)\n", name, hdr.x, hdr.y);
		err = -5;
	}
	else if (hdr.bpp != 24 && hdr.bpp != 32) {
		printf("\"%s\" is not a 24-bit or 32-bit BMP (ndp: %d)\n", name, hdr.ndp);
		err = -6;
	}
	else if (hdr.comp) {
		printf("BMP Compression is not supported\n");
		err = -7;
	}
	if (err) {
		free(buf);
		return err;
	}

	int xr = hdr.x * (hdr.bpp / 8);
	xr += (4 - (xr % 4)) % 4;
	if (sz != hdr.off + xr * hdr.y) {
		printf("The size of \"%s\" (%d) does not match its projected size (%d)\n"
		       "(offset: %d, bpp: %d, row length: %d, x: %d, y: %d)\n",
		       name, sz, hdr.off + xr * hdr.y, hdr.off, hdr.bpp, xr, hdr.x, hdr.y);
		free(buf);
		return -8;
	}

	if (tex->img) free(tex->img);
	tex->img = calloc(hdr.x * hdr.y, sizeof(TRIM_Color));

	int i, j;
	for (i = 0; i < hdr.y; i++) {
		u8 *ptr = buf + (hdr.y-i-1) * xr + hdr.off;
		for (j = 0; j < xr; j += hdr.bpp/8) {
			TRIM_Color p = {ptr[j+2], ptr[j+1], ptr[j], hdr.bpp == 32 ? ptr[j+3] : 0xff};
			memcpy(tex->img + (i * hdr.x) + (j / (hdr.bpp/8)), &p, sizeof(TRIM_Color));
		}
	}

	tex->w = hdr.x;
	tex->h = hdr.y;
	free(buf);
	return 0;
}
