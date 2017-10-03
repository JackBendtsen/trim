#include "../include/trim.h"

#ifndef _WIN32_

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

int TRIM_16to256(int x) {
	if (x < 0 || x > 15) return 0;

	int p[] = {
		0x10, 0x12, 0x1c, 0x1e,
		0x58, 0x5a, 0x64, 0xfa,
		0xf4, 0x15, 0x2e, 0x33,
		0xc4, 0xc9, 0xe2, 0xe7
	};
	return p[x];
}

#endif

void TRIM_BlendColor(TRIM_Color *dst, TRIM_Color *src) {
	if (!dst) return;

	u8 *dst_pixel = (u8*)dst;
	u8 src_pixel[4] = {0};
	//float dst_a;
	float src_alpha;
	if (src) {
		memcpy(&src_pixel, src, sizeof(TRIM_Color));
		//dst_a = (float)dst->a / 255.0;
		src_alpha = (float)src->a / 255.0;
	}
	else {
		//dst_a = 1.0;
		src_alpha = (float)(255 - dst->a) / 255.0;
	}

	int i;
	for (i = 0; i < 3; i++) {
		float dst_chn = (float)(dst_pixel[i]) / 255.0;
		float src_chn = (float)(src_pixel[i]) / 255.0;

		dst_chn += src_chn * src_alpha;
		if (dst_chn < 0.0) dst_chn = 0.0;
		if (dst_chn > 1.0) dst_chn = 1.0;

		dst_pixel[i] = (u8)(dst_chn * 255.0);
	}
}

void TRIM_CreatePixel(TRIM_Pixel *p, TRIM_Color *bg, TRIM_Color *fg) {
	if (!p) return;

	TRIM_Color b = {0, 0, 0, 0xff};
	TRIM_Color f = {0xff, 0xff, 0xff, 0xff};
	if (bg) memcpy(&b, bg, sizeof(TRIM_Color));
	if (fg) memcpy(&f, fg, sizeof(TRIM_Color));

	memcpy(&p->bg, &b, sizeof(TRIM_Color));
	memcpy(&p->fg, &f, sizeof(TRIM_Color));
}

TRIM_Sprite *TRIM_CreateSprite(int w, int h, int x, int y) {
	if (w < 1 || h < 1 || x < 0 || y < 0) return NULL;

	TRIM_Sprite *spr = calloc(1, sizeof(TRIM_Sprite));
	spr->w = w;
	spr->h = h;
	spr->x = x;
	spr->y = y;

	TRIM_Pixel p = {0};
	TRIM_CreatePixel(&p, NULL, NULL);
	TRIM_FillPixelBuffer(spr, &p);

	return spr;
}

void TRIM_CloseSprite(TRIM_Sprite *s) {
	if (!s) return;
	if (s->pix) free(s->pix);
	if (s->ch) free(s->ch);
	memset(s, 0, sizeof(TRIM_Sprite));
	free(s);
}

void TRIM_FillPixelBuffer(TRIM_Sprite *spr, TRIM_Pixel *p) {
	if (!spr) return;

	int ssz = spr->w * spr->h;
	if (!spr->pix) spr->pix = calloc(ssz, sizeof(TRIM_Pixel));

	TRIM_Pixel pixel = {0};
	if (!p) TRIM_CreatePixel(&pixel, NULL, NULL);
	else memcpy(&pixel, p, sizeof(TRIM_Pixel));

	int i;
	for (i = 0; i < ssz; i++) memcpy(&spr->pix[i], &pixel, sizeof(TRIM_Pixel));
}

void TRIM_FillCharBuffer(TRIM_Sprite *spr, char c) {
	if (!spr || spr->w < 1 || spr->h < 1) return;

	int spr_sz = spr->w * spr->h;
	if (!spr->ch) spr->ch = calloc(spr_sz + 1, 1);

	if (c < 0x20 || c > 0x7e) c = ' ';
	memset(spr->ch, c, spr_sz);
}

void TRIM_ApplyText(TRIM_Sprite *spr, char *text, int x, int y) {
	if (!text || !strlen(text)) return;
	if (!spr) spr = TRIM_Screen;

	int pos = (y * spr->w) + x;
	int off = 0;
	int ssz = spr->w * spr->h;
	int len = strlen(text);

	if (pos <= -len || pos >= ssz) return;
	if (pos < 0) {
		off = -pos;
		pos = 0;
	}

	if (!spr->ch) TRIM_FillCharBuffer(spr, ' ');

	int i;
	for (i = pos; i < ssz && i < pos + (len - off); i++) {
		spr->ch[i] = text[i-pos];
	}
}

void TRIM_ApplyTextBox(TRIM_Sprite *spr, char *text, int pos, int x, int y, int w, int h) {
	if (!text || !strlen(text) || pos < 0 || pos >= strlen(text)) return;

	if (!spr) spr = TRIM_Screen;
	if (w < 1 || h < 1 || x <= -w || x >= spr->w || y <= -h || y >= spr->h) return;

	if (!spr->ch) TRIM_FillCharBuffer(spr, ' ');

	int i;
	for (i = pos; i < w*h && i < pos+strlen(text); i++) {
		spr->ch[(y + (i / w)) * w + (x + (i % w))] = text[i-pos];
	}
}

void TRIM_ApplySprite(TRIM_Sprite *dst, TRIM_Sprite *src) {
	if (!dst || !src || !dst->pix || !src->pix) return;
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

	char *sc = src->ch;
	char *dc = dst->ch;

	int i, j;
	for (i = 0; i < h; i++) {
		for (j = 0; j < w; j++) {
			TRIM_Pixel *dp = &dst->pix[(dy+i) * dst->w + dx+j];
			TRIM_Pixel *sp = &src->pix[(sy+i) * src->w + sx+j];

			TRIM_BlendColor(&dp->bg, &sp->bg);
			TRIM_BlendColor(&dp->fg, &sp->fg);

			if (sc && dc && *sc > 0x1f && *sc < 0x7f) *dc++ = *sc++;
		}
	}
}

void TRIM_ResizeSprite(TRIM_Sprite *spr, int w, int h) {
	if (!spr || w < 1 || h < 1 || (spr->w == w && spr->h == h)) return;

	spr->pix = realloc(spr->pix, w * h * sizeof(TRIM_Pixel));
	spr->ch = realloc(spr->ch, w * h);

	TRIM_Pixel ep = {0};
	TRIM_CreatePixel(&ep, NULL, NULL);

	int i, j;
	if (w > spr->w) {
		for (i = h-1; i >= 0; i--) {
			if (i) {
				memmove(spr->pix + i*w, spr->pix + i*spr->w, spr->w * sizeof(TRIM_Pixel));
				memmove(spr->ch + i*w, spr->ch + i*spr->w, spr->w);
			}
			for (j = spr->w; j < w; j++) {
				memcpy(&spr->pix[i*w + j], &ep, sizeof(TRIM_Pixel));
			}
			memset(spr->ch + i*w + spr->w, ' ', w - spr->w);
		}
	}
	if (h > spr->h) {
		for (i = spr->h; i < h; i++) {
			for (j = 0; j < w; j++) {
				memcpy(&spr->pix[i*w + j], &ep, sizeof(TRIM_Pixel));
			}
		}
		memset(spr->ch + w * spr->h, ' ', w * (h - spr->h));
	}
	spr->w = w;
	spr->h = h;
}

void TRIM_ApplyTextureToSprite(TRIM_Sprite *spr, TRIM_Texture *tex, int x, int y, int w, int h) {
	if (!tex) return;
	if (!spr) spr = TRIM_Screen;

	TRIM_Texture sc_tex = {0};
	TRIM_ScaleTexture(&sc_tex, tex, w, h);

	TRIM_Sprite new_spr = {0}; // names are hard

	new_spr.x = x;
	new_spr.y = y;
	new_spr.w = sc_tex.w;
	new_spr.h = sc_tex.h;
	new_spr.pix = calloc(sc_tex.w * sc_tex.h, sizeof(TRIM_Pixel));
	// no char buffer needed

	int i;
	for (i = 0; i < sc_tex.w * sc_tex.h; i++) {
		memcpy(&new_spr.pix[i].bg, &sc_tex.img[i], sizeof(TRIM_Color));
		memset(&new_spr.pix[i].fg, 0, sizeof(TRIM_Color));
	}
	free(sc_tex.img);

	if (!spr->pix) {
		memcpy(spr, &new_spr, sizeof(TRIM_Sprite));
		return;
	}

	TRIM_ApplySprite(spr, &new_spr);
	free(new_spr.pix);
}

typedef struct {
	float r, g, b, a;
} tclf;

struct scaler {
	int size;
	int len;
	float factor;

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

/*
FILE *debug = NULL;

void debug_colour(TRIM_Color *c, char *name) {
	if (!debug) return;
	fprintf(debug,
		"%s:\n"
		"  r = %d\n"
		"  g = %d\n"
		"  b = %d\n"
		"  a = %d\n",
		name ? name : "colour", c->r, c->g, c->b, c->a);


}

void debug_pixel(TRIM_Pixel *p) {
	if (!debug) return;
	debug_colour(&p->bg, "bg");
	debug_colour(&p->fg, "fg");
	fprintf(debug, "ch: '%c'\n\n", p->ch);
}
*/

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
Note: this struct does not include the BMP signature as it is a 2-byte field by itself,
      thus screwing up the entire struct's byte alignment
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