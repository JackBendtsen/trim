#include "trim.h"

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

void trim_rendertexture(tsprite *spr, ttexture *tex, int x, int y, int w, int h) {
	if (!spr || !tex) return;

	ttexture sc_tex = {0};
	trim_scaletexture(&sc_tex, tex, w, h);

	tsprite new_spr = {0}; // names are hard

	new_spr.x = x;
	new_spr.y = y;
	new_spr.w = sc_tex.w;
	new_spr.h = sc_tex.h;
	new_spr.mode = TGFX_RGB;
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

void gotoyx(int y, int x) {
#ifdef _WIN32_
	COORD pos = {x, y};
	SetConsoleCursorPosition(trim_wch, pos);
#else
	printf("\x1b[%d;%dH", y+1, x+1);
#endif
}

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
			if (s->mode == TGFX_16) {
				bg = trim_16to256(trim_to16(&s->pix[p].bg));
				fg = trim_16to256(trim_to16(&s->pix[p].fg));
				printf("\x1b[48;5;%dm\x1b[38;5;%dm", bg, fg);
			}
			else if (s->mode == TGFX_256) {
				bg = trim_to256(&s->pix[p].bg);
				fg = trim_to256(&s->pix[p].fg);
				printf("\x1b[48;5;%dm\x1b[38;5;%dm", bg, fg);
			}
			else if (s->mode == TGFX_RGB) {
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

void trim_resizescreen(void) {
	trim_old_w = trim_screen->w;
	trim_old_h = trim_screen->h;

	int w = 0, h = 0;
	trim_getconsolesize(&w, &h);
	trim_resizesprite(trim_screen, w, h);
}

void trim_getconsolesize(int *w, int *h) {
#ifdef _WIN32_
	CONSOLE_SCREEN_BUFFER_INFO csbi;
	GetConsoleScreenBufferInfo(trim_wch, &csbi);

	if (w) *w = csbi.srWindow.Right - csbi.srWindow.Left + 1;
	if (h) *h = csbi.srWindow.Bottom - csbi.srWindow.Top + 1;
#else
	struct winsize win;
	ioctl(0, TIOCGWINSZ, &win);

	if (w) *w = win.ws_col;
	if (h) *h = win.ws_row;
#endif
}

void trim_initvideo(int mode) {
	//trim_wch = CreateConsoleScreenBuffer(GENERIC_READ | GENERIC_WRITE, 0, NULL, CONSOLE_TEXTMODE_BUFFER, NULL);

	COORD pos = {0, 0};
	ReadConsoleOutputAttribute(trim_wch, &trim_old_colour, 1, pos, NULL);

	trim_getconsolesize(&trim_old_w, &trim_old_h);
	trim_screen = trim_createsprite(trim_old_w, trim_old_h, 0, 0, mode);
}

#else

void resize_screen(int signo) {
	if (signo != SIGWINCH) return;
	trim_resizescreen();
}

void trim_initvideo(int mode) {
	printf("\x1b[?25l\x1b[?7l");

	signal(SIGWINCH, resize_screen);

	trim_getconsolesize(&trim_old_w, &trim_old_h);
	trim_screen = trim_createsprite(trim_old_w, trim_old_h, 0, 0, mode);
}

#endif

void trim_closevideo(int keep_screen) {
	if (!keep_screen) {
		tsprite *blank = trim_createsprite(trim_screen->w, trim_screen->h, 0, 0, trim_screen->mode);
		trim_drawsprite(blank);
		trim_closesprite(blank);
	}
#ifdef _WIN32_
	CloseHandle(trim_wch);
#else
	printf("\x1b[?7h\x1b[?25h\n");
#endif
	trim_closesprite(trim_screen);
}
