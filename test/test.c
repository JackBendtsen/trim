#include "tgfx.h"
#include <unistd.h>

#define WIN_W 100
#define WIN_H 30

#define WIDTH 80
#define HEIGHT 25

int main() {
	ttexture *screen = trim_initvideo(WIN_W, WIN_H, WIDTH, HEIGHT, MODE_256);
	tcolour bg = {24, 24, 24, 0xff};
	tcolour fg = {0xff, 0xff, 0xff, 0xff};
	tpixel p = {0};
	trim_createpixel(&p, &bg, &fg, ':');

	int c = 200;
	while (c >= 0) {
		p.fg.a--;
		trim_filltexture(screen, &p);
		trim_drawtexture(screen);
		usleep(10000);
		c--;
	}

	trim_closevideo(screen);
	free(screen);
	return 0;
}