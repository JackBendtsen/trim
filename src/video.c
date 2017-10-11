#include "../include/trim.h"

#ifdef _WIN32
  HANDLE TRIM_wch;
#endif

int TGFX_mode = 0;

TRIM_Sprite *TRIM_Screen = NULL;
int TRIM_old_w = 0;
int TRIM_old_h = 0;

#ifdef _WIN32

void TRIM_DrawScreen(void) {
	TRIM_Sprite *s = TRIM_Screen;
	if (!s) return;

	TRIM_ResizeScreen();

	// Print the characters to the screen first
	int size = s->w * s->h;
	COORD pos = {0, 0};
	if (s->ch) WriteConsoleOutputCharacter(TRIM_wch, s->ch, size, pos, NULL);

	short *colours = calloc(size, sizeof(short));
	int i, bg, fg;

	// Then get a list of the colours of each pixel
	for (i = 0; i < size; i++) {
		TRIM_BlendColor(&s->pix[i].bg, NULL);
		TRIM_BlendColor(&s->pix[i].fg, NULL);

		bg = TRIM_to16(&s->pix[i].bg);
		fg = TRIM_to16(&s->pix[i].fg);
		colours[i] = (bg << 4) | fg;
	}

	WriteConsoleOutputAttribute(TRIM_wch, colours, size, pos, NULL);
	free(colours);
}

#else

inline void byte2str(char *str, int byte) {
	str[0] = '0' + ((byte / 100) % 10);
	str[1] = '0' + ((byte / 10) % 10);
	str[2] = '0' + (byte % 10);
}

void TRIM_DrawScreen(void) {
	TRIM_Sprite *s = TRIM_Screen;
	if (!s) return;

	TRIM_ResizeScreen();

	char *print_pos = strdup("\x1b[000;000H");
	char *print_256 = strdup("\x1b[48;5;000m\x1b[38;5;000m");
	char *print_rgb = strdup("\x1b[48;2;000;000;000m\x1b[38;2;000;000;000m");

	char c = 0;
	int i, j, p = 0;
	for (i = 0; i < s->h; i++) {
		// Set console cursor position
		byte2str(print_pos + 2, s->y + i + 1);
		byte2str(print_pos + 6, s->x + 1);
		write(1, print_pos, 10);

		for (j = 0; j < s->w; j++, p++) {
			TRIM_BlendColor(&s->pix[p].bg, NULL);
			TRIM_BlendColor(&s->pix[p].fg, NULL);

			int bg = 0, fg = 0;
			if (TGFX_mode == TGFX_16) {
				// Print 16-colour "pixel"
				byte2str(print_256 + 7, TRIM_16to256(TRIM_to16(&s->pix[p].bg)));
				byte2str(print_256 + 18, TRIM_16to256(TRIM_to16(&s->pix[p].fg)));
				write(1, print_256, 22);
			}
			else if (TGFX_mode == TGFX_256) {
				// Print 256-colour "pixel"
				byte2str(print_256 + 7, TRIM_to256(&s->pix[p].bg));
				byte2str(print_256 + 18, TRIM_to256(&s->pix[p].fg));
				write(1, print_256, 22);
			}
			else if (TGFX_mode == TGFX_RGB) {
				// Print RGB "pixel"
				byte2str(print_rgb + 7, s->pix[p].bg.r);
				byte2str(print_rgb + 11, s->pix[p].bg.g);
				byte2str(print_rgb + 15, s->pix[p].bg.b);
				byte2str(print_rgb + 26, s->pix[p].fg.r);
				byte2str(print_rgb + 30, s->pix[p].fg.g);
				byte2str(print_rgb + 34, s->pix[p].fg.b);
				write(1, print_rgb, 38);
			}

			c = ' ';
			if (s->ch && s->ch[p] > ' ' && s->ch[p] <= '~') c = s->ch[p];
			write(1, &c, 1);
			//fflush(stdout);
		}
		write(1, "\x1b[0m", 4);
		//fflush(stdout);
	}

	free(print_pos);
	free(print_256);
	free(print_rgb);
}

#endif

void TRIM_ClearScreen(void) {
	TRIM_FillPixelBuffer(TRIM_Screen, NULL);
	TRIM_FillCharBuffer(TRIM_Screen, 0);
}

void TRIM_ResizeScreen(void) {
	TRIM_old_w = TRIM_Screen->w;
	TRIM_old_h = TRIM_Screen->h;

	int w = 0, h = 0;
	TRIM_GetConsoleSize(&w, &h);
	TRIM_ResizeSprite(TRIM_Screen, w, h);
}

TRIM_Sprite *TRIM_GetScreen(void) {
	return TRIM_Screen;
}

void TRIM_GetConsoleSize(int *w, int *h) {
#ifdef _WIN32
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

int TRIM_SetConsoleSize(int w, int h) {
	return 0;
}

void TRIM_InitVideo(int mode) {
#ifdef _WIN32
	TRIM_wch = CreateConsoleScreenBuffer(GENERIC_READ | GENERIC_WRITE, 0, NULL, CONSOLE_TEXTMODE_BUFFER, NULL);
	SetConsoleActiveScreenBuffer(TRIM_wch);
	TGFX_mode = TGFX_16;
/*
	COORD pos = {0, 0};
	ReadConsoleOutputAttribute(TRIM_wch, &TRIM_old_colour, 1, pos, NULL);
*/
#else
	printf("\x1b[?25l\x1b[?7l");

	mode = mode < 0 ? 0 : mode;
	TGFX_mode = mode > 2 ? 2 : mode;
#endif
	TRIM_GetConsoleSize(&TRIM_old_w, &TRIM_old_h);
	TRIM_Screen = TRIM_CreateSprite(TRIM_old_w, TRIM_old_h, 0, 0);
	TRIM_DrawScreen();
}

void TRIM_CloseVideo(int keep_screen) {
	if (!keep_screen) {
		TRIM_ClearScreen();
		TRIM_DrawScreen();
	}
	TRIM_CloseSprite(TRIM_Screen);

#ifdef _WIN32
	SetConsoleActiveScreenBuffer(GetStdHandle(STD_OUTPUT_HANDLE));
	CloseHandle(TRIM_wch);
#else
	printf("\x1b[?7h\x1b[?25h\n");
#endif
}

void debug_ptr(char *file, char *name, void *ptr) {
	if (!file) return;
	FILE *f = fopen(file, "w");
	fprintf(f, "%s: %p", name ? name : "TRIM_Screen", name ? ptr : TRIM_Screen);
	fclose(f);
}