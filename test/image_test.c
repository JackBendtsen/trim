#include "../tgfx.h"
#include "../tkb.h"

int main() {
	trim_initkb(TRIM_DEFKB);
	trim_initvideo(TRIM_RGB);

	ttexture tex = {0};
	if (trim_openbmp(&tex, "image_test.bmp") < 0) return 1;

	int x = 0, y = 0, w = 0, h = 0, mode = 0, key = 0, fc = 0;
	while (1) {
		if (key == 0x1b) break;
		if (key == ' ') mode = !mode;

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

/*
		if (x < 0) x = 0;
		if (x >= WIDTH) x = WIDTH-1;

		if (y < 0) y = 0;
		if (y >= HEIGHT) y = HEIGHT-1;

		if (w < 0) w = 0;
		if (w >= WIDTH) w = WIDTH-1;

		if (h < 0) h = 0;
		if (h >= HEIGHT) h = HEIGHT-1;
*/

		trim_fillsprite(trim_screen, NULL);
		trim_rendertexture(trim_screen, &tex, x, y, w+1, h+1);

		char msg[48];
		sprintf(msg, "x: %d, y: %d, w: %d, h: %d, W: %d, H: %d, fc: %d", x, y, w+1, h+1, trim_screen->w, trim_screen->h, fc);
		trim_printsprite(trim_screen, msg, 0, 0, 1);
		trim_drawsprite(trim_screen);

		key = trim_getkey();
		fc++;
		//if (key < 0) break;
	}

	free(tex.img);
	trim_closevideo();
	trim_closekb();
	return 0;
}
