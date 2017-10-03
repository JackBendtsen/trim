#include "../include/trim.h"

int main() {
	TRIM_InitKB(TRIM_DEFKB);
	TRIM_InitVideo(TGFX_RGB);

	TRIM_Texture tex = {0};
	if (TRIM_OpenBMP(&tex, "image_test.bmp") < 0) return 1;

	int x = 0, y = 0, w = 0, h = 0, cw = 0, ch = 0, mode = 0, key = 0, fc = 0;
	while (1) {
		if (key == TKEY_ESC) break;
		if (key == TKEY_SPACE) mode = !mode;

		if (key == TKEY_UP) {
			if (mode) y--;
			else h--;
		}
		if (key == TKEY_DOWN) {
			if (mode) y++;
			else h++;
		}
		if (key == TKEY_LEFT) {
			if (mode) x--;
			else w--;
		}
		if (key == TKEY_RIGHT) {
			if (mode) x++;
			else w++;
		}

		TRIM_ClearScreen();
		TRIM_ApplyTextureToSprite(NULL, &tex, x, y, w+1, h+1);

		char msg[48];
		TRIM_GetConsoleSize(&cw, &ch);
		sprintf(msg, "x: %d, y: %d, w: %d, h: %d, W: %d, H: %d, fc: %d", x, y, w+1, h+1, cw, ch, fc);
		TRIM_ApplyText(NULL, msg, 0, 0);
		TRIM_DrawScreen();

		key = TRIM_GetKey();
		fc++;
		//if (key < 0) break;
	}

	free(tex.img);
	TRIM_CloseVideo(0);
	TRIM_CloseKB();
	return 0;
}
