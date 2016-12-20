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

int trim_to16(tcolour *c) {
	int r = ((int)c->r * (int)c->a) / 0xff;
	int g = ((int)c->g * (int)c->a) / 0xff;
	int b = ((int)c->b * (int)c->a) / 0xff;

	int i, match[16];
	for (i = 0; i < 16; i++) {
		int rd = r - _trim_16cp[i].r;
		int gd = g - _trim_16cp[i].g;
		int bd = b - _trim_16cp[i].b;
		if (rd < 0) rd = -rd;
		if (gd < 0) gd = -gd;
		if (bd < 0) bd = -bd;
		match[i] = rd + gd + bd;
	}

	int low = match[0], idx = 0;
	for (i = 1; i < 16; i++) {
		if (match[i] < low) {
			low = match[i];
			idx = i;
		}
	}
	return idx;
}

int trim_16to256(int x) {
	if (x < 0 || x > 15) return 0;

	int p[] = {
		0x10, 0x12, 0x1c, 0x1e,
		0x58, 0x5a, 0x64, 0xfa,
		0xf4, 0x15, 0x2e, 0x33,
		0xc4, 0xc9, 0xe2, 0xe7
	};
	return p[x];
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

	int i, j;
	for (i = 0; i < h; i++) {
		for (j = 0; j < w; j++) {
			tpixel *dp = &dst->pix[(dy+i) * dst->w + dx+j];
			tpixel *sp = &src->pix[(sy+i) * src->w + sx+j];

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

#ifdef _WIN32_

void trim_drawsprite(tsprite *s) {
	if (!s) return;
	int i, j, p = 0, bg, fg;
	for (i = 0; i < s->h; i++) {
		for (j = 0; j < s->w; j++, p++) {
			trim_blendcolour(&s->pix[p].bg, NULL);
			trim_blendcolour(&s->pix[p].fg, NULL);

			bg = trim_to16(&s->pix[p].bg);
			fg = trim_to16(&s->pix[p].fg);
			int attr = (bg << 4) | fg;
			
			COORD pos = {j, i};
			WriteConsoleOutputAttribute(trim_wch, &attr, 1, pos, NULL);
			WriteConsoleOutputCharacter(trim_wch, &s->pix[p].ch, 1, pos, NULL);
		}
	}
}

#else

void trim_drawsprite(tsprite *s) {
	if (!s) return;
	int i, j, p = 0;
	for (i = 0; i < s->h; i++) {
		printf("\x1b[%d;%dH", s->y+i+1, s->x+1);

		for (j = 0; j < s->w; j++, p++) {
			trim_blendcolour(&s->pix[p].bg, NULL);
			trim_blendcolour(&s->pix[p].fg, NULL);

			int bg = 0, fg = 0;
			if (s->mode == TRIM_16) {
				bg = trim_16to256(trim_to16(&s->pix[p].bg));
				fg = trim_16to256(trim_to16(&s->pix[p].fg));
				printf("\x1b[48;5;%dm\x1b[38;5;%dm", bg, fg);
			}
			else if (s->mode == TRIM_256) {
				bg = trim_to256(&s->pix[p].bg);
				fg = trim_to256(&s->pix[p].fg);
				printf("\x1b[48;5;%dm\x1b[38;5;%dm", bg, fg);
			}
			else if (s->mode == TRIM_RGB) {
				printf("\x1b[48;2;%d;%d;%dm", s->pix[p].bg.r, s->pix[p].bg.g, s->pix[p].bg.b);
				printf("\x1b[38;2;%d;%d;%dm", s->pix[p].fg.r, s->pix[p].fg.g, s->pix[p].fg.b);
			}

			char c = s->pix[p].ch;
			putchar((c > 0x1f && c < 0x7f) ? c : ' ');
			//fflush(stdout);
		}
		printf("\x1b[0m");
		fflush(stdout);
	}
}

#endif

void trim_resizesprite(tsprite *s, int w, int h) {
	if (!s || w < 1 || h < 1) return;

	s->pix = realloc(s->pix, w * h * sizeof(tpixel));

	tpixel ep = {0};
	trim_createpixel(&ep, NULL, NULL, ' ');
	int i, j;
	if (w > s->w) {
		for (i = h-1; i >= 0; i--) {
			if (i) memmove(s->pix + i*w, s->pix + i*s->w, s->w * sizeof(tpixel));
			for (j = s->w; j < w; j++) {
				memcpy(&s->pix[i*w + j], &ep, sizeof(tpixel));
			}
		}
	}
	if (h > s->h) {
		for (i = s->h; i < h; i++) {
			for (j = 0; j < w; j++) {
				memcpy(&s->pix[i*w + j], &ep, sizeof(tpixel));
			}
		}
	}
	s->w = w;
	s->h = h;
}

void trim_closesprite(tsprite *s) {
	if (!s) return;
	if (s->pix) free(s->pix);
	memset(s, 0, sizeof(tsprite));
	free(s);
}

#ifdef _WIN32_

void trim_getconsolesize(int *w, int *h) {
	CONSOLE_SCREEN_BUFFER_INFO csbi;
    GetConsoleScreenBufferInfo(trim_wch, &csbi);

    if (w) *w = csbi.srWindow.Right - csbi.srWindow.Left + 1;
    if (h) *h = csbi.srWindow.Bottom - csbi.srWindow.Top + 1;
}

void trim_initvideo(int mode) {
	//trim_win32conhnd = CreateConsoleScreenBuffer(GENERIC_READ | GENERIC_WRITE, 0, NULL, CONSOLE_TEXTMODE_BUFFER, NULL);
	trim_wch = GetStdHandle(STD_OUTPUT_HANDLE);

	COORD pos = {0, 0};
	ReadConsoleOutputAttribute(trim_wch, &trim_old_colour, 1, pos, NULL);

	trim_getconsolesize(&trim_old_w, &trim_old_h);
	trim_screen = trim_createsprite(trim_old_w, trim_old_h, 0, 0, mode);
}

void trim_closevideo(void) {
	int size = trim_screen->w * trim_screen->h;
	COORD pos = {0, 0};
	FillConsoleOutputAttribute(trim_wch, trim_old_colour, size, pos, NULL);
	FillConsoleOutputCharacter(trim_wch, ' ', size, pos, NULL);

	CloseHandle(trim_wch);
	trim_closesprite(trim_screen);
}

#else

void resize_screen(int signo) {
	if (signo != SIGWINCH) return;

	trim_old_w = trim_screen->w;
	trim_old_h = trim_screen->h;

	struct winsize win;
	ioctl(0, TIOCGWINSZ, &win);

	trim_resizesprite(trim_screen, win.ws_col, win.ws_row);
}

void trim_initvideo(int mode) {
	printf("\x1b[?25l");
	printf("\x1b[?7l");

	signal(SIGWINCH, resize_screen);

	struct winsize win;
	ioctl(0, TIOCGWINSZ, &win);
	trim_old_w = win.ws_col;
	trim_old_h = win.ws_row;

	trim_screen = trim_createsprite(trim_old_w, trim_old_h, 0, 0, mode);
}

void trim_closevideo(void) {
	printf("\x1b[0m");
	int i, j;
	for (i = 0; i < trim_screen->h; i++) {
		printf("\x1b[%d;1H", i+1);
		for (j = 0; j < trim_screen->w; j++) putchar(' ');
	}
	printf("\x1b[?7h");
	printf("\x1b[?25h\n");
	trim_closesprite(trim_screen);
}

#endif