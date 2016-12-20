#include "tgfx.h"

#define WIN_W 100
#define WIN_H 30

#define WIDTH 80
#define HEIGHT 25

void gen_numbers(int *buf, int low, int high, int size) {
	if (size < 1 || high <= low) return;
	int range = high - low;
	//int *rng = malloc(size * sizeof(int));

	FILE *f = fopen("/dev/urandom", "rb");
	if (!f) return;
	fread(buf, sizeof(int), size, f);
	fclose(f);

	int i;
	for (i = 0; i < size; i++) {
		buf[i] %= range + 1;
		buf[i] += low;
	}
}

int main() {
	ttexture *screen = trim_initvideo(WIN_W, WIN_H, WIDTH, HEIGHT, MODE_256);
	int size = WIDTH * HEIGHT;
	int *rng = calloc(size, sizeof(int));
	gen_numbers(rng, 0, 0x0fffffff, size * 2);

	int i;
	for (i = 0; i < size * 2; i += 2) {
		tcolour bg = {rng[i] >> 16, rng[i] >> 8, rng[i], 0xff};
		tcolour fg = {rng[i+1] >> 16, rng[i+1] >> 8, rng[i+1], 0xff};
		char ch = (char)((rng[i] >> 20) & 0x70);
		ch |= (char)(rng[i+1] >> 24);

		tpixel pix;
		trim_createpixel(&pix, &bg, &fg, ch);
		screen->pix[i] = pix;
	}
	free(rng);

	trim_drawtexture(screen);
	trim_closevideo(screen);
	free(screen);
	return 0;
}