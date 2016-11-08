#include "../tgfx.h"
#include "../tkb.h"

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

tcolour *openbmp(char *name, int *w, int *h) {
	if (!name || !w || !h) return NULL;

	FILE *f = fopen(name, "rb");
	if (!f) {
		printf("Could not open \"%s\"\n", name);
		return NULL;
	}

	fseek(f, 0, SEEK_END);
	int sz = ftell(f);
	rewind(f);
	if (sz < sizeof(bmp_t)) {
		printf("\"%s\" isn't big enough to be a BMP file (size: %d)\n", name, sz);
		return NULL;
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
		err = 1;
	}
	else if (hdr.x < 1 || hdr.y < 1) {
		printf("\"%s\" has invalid dimensions (%dx%d)\n", name, hdr.x, hdr.y);
		err = 1;
	}
	else if (hdr.bpp != 24 && hdr.bpp != 32) {
		printf("\"%s\" is not a 24-bit or 32-bit BMP (ndp: %d)\n", name, hdr.ndp);
		err = 1;
	}
	else if (hdr.comp) {
		printf("Compression is not supported by this program\n");
		err = 1;
	}
	if (err) {
		free(buf);
		return NULL;
	}

	int xr = hdr.x * (hdr.bpp / 8);
	xr += (4 - (xr % 4)) % 4;
	if (sz != hdr.off + xr * hdr.y) {
		printf("The size of \"%s\" (%d) does not match its projected size (%d)\n"
		       "(offset: %d, bpp: %d, row length: %d, x: %d, y: %d)\n",
		       name, sz, hdr.off + xr * hdr.y, hdr.off, hdr.bpp, xr, hdr.x, hdr.y);
		free(buf);
		return NULL;
	}

	tcolour *img = calloc(hdr.x * hdr.y, sizeof(tcolour));
	int i, j;
	for (i = 0; i < hdr.y; i++) {
		u8 *ptr = buf + (hdr.y-i-1) * xr + hdr.off;
		for (j = 0; j < xr; j += hdr.bpp/8) {
			tcolour p = {ptr[j+2], ptr[j+1], ptr[j], hdr.bpp == 32 ? ptr[j+3] : 0xff};
			memcpy(img + (i * hdr.x) + (j / (hdr.bpp/8)), &p, sizeof(tcolour));
		}
	}
	*w = hdr.x;
	*h = hdr.y;
	free(buf);
	return img;
}

#define WIDTH 88
#define HEIGHT 26

int main() {
	trim_initkb(TRIM_DEFKB);

	int iw, ih;
	tcolour *img = openbmp("image_test.bmp", &iw, &ih);
	if (!img) return 1;

	ttexture tex = {img, iw, ih};
	tsprite *screen = trim_initvideo(100, 30, WIDTH, HEIGHT, TRIM_RGB);

	int x = 0, y = 0, w = 0, h = 0, mode = 0, key = 0;
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

		if (x < 0) x = 0;
		if (x >= WIDTH) x = WIDTH-1;

		if (y < 0) y = 0;
		if (y >= HEIGHT) y = HEIGHT-1;

		if (w < 0) w = 0;
		if (w >= WIDTH) w = WIDTH-1;

		if (h < 0) h = 0;
		if (h >= HEIGHT) h = HEIGHT-1;

		trim_fillsprite(screen, NULL);
		trim_rendertexture(screen, &tex, x, y, w+1, h+1);

		char msg[48];
		sprintf(msg, "x: %d, y: %d, w: %d, h: %d", x, y, w+1, h+1);
		trim_printsprite(screen, msg, 0, 0, 0);
		trim_drawsprite(screen);

		key = trim_getkey();
	}

	free(tex.img);
	trim_closevideo(screen);
	trim_closekb();
	return 0;
}
