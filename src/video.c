#include "trim.h"

void gotoyx(int y, int x) {
#ifdef _WIN32_
	COORD pos = {x, y};
	SetConsoleCursorPosition(TRIM_wch, pos);
#else
	printf("\x1b[%d;%dH", y+1, x+1);
#endif
}

#ifdef _WIN32_

void TRIM_DrawScreen(void) {
	TRIM_Sprite *s = TRIM_Screen;
	if (!s) return;

	// Print the characters to the screen first
	int size = s->w * s->h;
	COORD pos = {0, 0};
	if (s->ch) WriteConsoleOutputCharacter(TRIM_wch, s->ch, size, pos, NULL);

	u8 *colours = calloc(size, 1);
	int i, bg, fg;

	// Then get a list of the colours of each pixel
	for (i = 0; i < size; i++) {
		TRIM_BlendColor(&s->pix[i].bg, NULL);
		TRIM_BlendColor(&s->pix[i].fg, NULL);

		bg = TRIM_to16(&s->pix[i].bg);
		fg = TRIM_to16(&s->pix[i].fg);
		colours[i] = (bg << 4) | fg;
	}

	//  Finally, find write the colours in streaks of pixels
	// to reduce the number of calls to the Win32 console API
	int idx = 0;
	for (i = 0; i < size; i++) {
		if (i == size-1 || colours[i] != colours[i+1]) {
			pos = {idx % s->w, idx / s->w};
			WriteConsoleOutputAttribute(TRIM_wch, &colours[idx], i-idx+1, pos, NULL);
			idx = i+1;
		}
	}

	free(colours);
}

#else

void TRIM_DrawScreen(void) {
	TRIM_Sprite *s = TRIM_Screen;
	if (!s) return;

	char c = 0, *endl = NULL;
	int i, j, p = 0;
	for (i = 0; i < s->h; i++) {
		gotoyx(s->y + i, s->x);

		for (j = 0; j < s->w; j++, p++) {
			TRIM_BlendColor(&s->pix[p].bg, NULL);
			TRIM_BlendColor(&s->pix[p].fg, NULL);

			int bg = 0, fg = 0;
			if (TGFX_mode == TGFX_16) {
				bg = TRIM_16to256(TRIM_to16(&s->pix[p].bg));
				fg = TRIM_16to256(TRIM_to16(&s->pix[p].fg));
				printf("\x1b[48;5;%dm\x1b[38;5;%dm", bg, fg);
			}
			else if (TGFX_mode == TGFX_256) {
				bg = TRIM_to256(&s->pix[p].bg);
				fg = TRIM_to256(&s->pix[p].fg);
				printf("\x1b[48;5;%dm\x1b[38;5;%dm", bg, fg);
			}
			else if (TGFX_mode == TGFX_RGB) {
				printf("\x1b[48;2;%d;%d;%dm", s->pix[p].bg.r, s->pix[p].bg.g, s->pix[p].bg.b);
				printf("\x1b[38;2;%d;%d;%dm", s->pix[p].fg.r, s->pix[p].fg.g, s->pix[p].fg.b);
			}

			if (s->ch) {
				c = s->ch[p];
				putchar((c > 0x1f && c < 0x7f) ? c : ' ');
			}
			else putchar(' ');
			//fflush(stdout);
		}
		printf("\x1b[0m");
		fflush(stdout);
	}
}

#endif

void TRIM_CloseSprite(TRIM_Sprite *s) {
	if (!s) return;
	if (s->pix) free(s->pix);
	if (s->ch) free(s->ch);
	memset(s, 0, sizeof(TRIM_Sprite));
	free(s);
}

void TRIM_ResizeScreen(void) {
	TRIM_old_w = TRIM_screen->w;
	TRIM_old_h = TRIM_screen->h;

	int w = 0, h = 0;
	TRIM_GetConsoleSize(&w, &h);
	TRIM_ResizeSprite(TRIM_screen, w, h);
}

void TRIM_GetConsoleSize(int *w, int *h) {
#ifdef _WIN32_
	CONSOLE_SCREEN_BUFFER_INFO csbi;
	GetConsoleScreenBufferInfo(TRIM_wch, &csbi);

	if (w) *w = csbi.srWindow.Right - csbi.srWindow.Left + 1;
	if (h) *h = csbi.srWindow.Bottom - csbi.srWindow.Top + 1;
#else
	struct winsize win;
	ioctl(0, TIOCGWINSZ, &win);

	if (w) *w = win.ws_col;
	if (h) *h = win.ws_row;
#endif
}

void TRIM_InitVideo(int mode) {
#ifdef _WIN32_
	TRIM_wch = CreateConsoleScreenBuffer(GENERIC_READ | GENERIC_WRITE, 0, NULL, CONSOLE_TEXTMODE_BUFFER, NULL);
	TGFX_mode = TGFX_16;
/*
	COORD pos = {0, 0};
	ReadConsoleOutputAttribute(TRIM_wch, &TRIM_old_colour, 1, pos, NULL);
*/
#else
	printf("\x1b[?25l\x1b[?7l");
	signal(SIGWINCH, resize_screen);

	mode = mode < 0 ? 0 : mode;
	TGFX_mode = mode > 2 ? 2 : mode;
#endif
	TRIM_GetConsoleSize(&TRIM_old_w, &TRIM_old_h);
	TRIM_Screen = TRIM_CreateSprite(TRIM_old_w, TRIM_old_h, 0, 0);
}

void TRIM_CloseVideo(int keep_screen) {
	if (!keep_screen) {
		TRIM_FillPixelBuffer(TRIM_Screen, NULL, NULL);
		free(TRIM_Screen->ch);
		TRIM_Screen->ch = NULL;
		TRIM_DrawScreen();
	}
	TRIM_CloseSprite(TRIM_Screen);

#ifdef _WIN32_
	CloseHandle(TRIM_wch);
#else
	printf("\x1b[?7h\x1b[?25h\n");
#endif
}
