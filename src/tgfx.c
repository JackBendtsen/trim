#include "trim.h"

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

int trim_openbmp(ttexture *tex, char *name) {
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
	tex->img = calloc(hdr.x * hdr.y, sizeof(tcolour));

	int i, j;
	for (i = 0; i < hdr.y; i++) {
		u8 *ptr = buf + (hdr.y-i-1) * xr + hdr.off;
		for (j = 0; j < xr; j += hdr.bpp/8) {
			tcolour p = {ptr[j+2], ptr[j+1], ptr[j], hdr.bpp == 32 ? ptr[j+3] : 0xff};
			memcpy(tex->img + (i * hdr.x) + (j / (hdr.bpp/8)), &p, sizeof(tcolour));
		}
	}
	tex->w = hdr.x;
	tex->h = hdr.y;
	free(buf);
	return 0;
}

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

void trim_scaletexture(ttexture *dst, ttexture *src, int w, int h) {
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
