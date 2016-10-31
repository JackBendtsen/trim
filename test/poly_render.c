#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef unsigned char u8;

typedef struct {
	u8 r, g, b, a;
} tcolour;

typedef struct {
	float tx, ty; // Coordinates of a point as mapped on a texture
	float sx, sy; // Coordinates of a point as mapped to the screen
} tvertex;

typedef struct {
	tcolour *pix;
	int x, y;
	tvertex vertex[4];
	int n_vertices;
} tpolygon;

typedef struct {
	
} bmp_t;

tcolour *openbmp(char *name, int *w, int *h) {
	if (!name || !w || !h) return -1;

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
	memcpy(&hdr, buf, sizeof(bmp_t));
	if (!memcmp(hdr.magic, "BM", 2)) {
		printf("\"%s\" is not a BMP file\n", name);
		free(buf);
		return NULL;
	}

	if (hdr.bpp != 24 && hdr.bpp != 32) {
		printf("\"%s\" is not a 24-bit or 32-bit BMP (bpp: %d)\n", name, hdr.bpp);
		free(buf);
		return NULL;
	}

	if (hdr.x < 1 || hdr.y < 1) {
		printf("\"%s\" has invalid dimensions (%dx%d)\n", name, hdr.x, hdr.y);
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
	int i, j, o = bpp == 24 ? -1 : 0;
	for (i = 0; i < hdr.y; i++) {
		u8 *ptr = buf + (hdr.y-i-1) * xr + hdr.off;
		for (j = 0; j < xr; j += hdr.bpp/8) {
			tcolour p = {o ? 0xff : ptr[j], ptr[j+o+1], ptr[j+o+2], ptr[j+o+3]};
			memcpy(img + (ptr-buf-hdr.off) + (j / (hdr.bpp/8)), &p, sizeof(tcolour));
		}
	}
	*w = hdr.x;
	*h = hdr.y;
	free(buf);
	return img;
}

void savebmp(char *name, tcolour *img, int w, int h) {
	if (!name || !img) return;
	
}

tcolour *trim_renderpolygon(tpolygon *p, int *rx, int *ry) {
	if (!p || !rx || !ry) return NULL;

	if (p->n_vertices < 3 || p->n_vertices > 4) {
		printf("Error: invalid number of vertices (%d)\n", p->n_vertices);
		return NULL;
	}

	int i;
	for (i = 0; i 

float *get_coordset(char *str, int *s) {
	if (!str || !s) return NULL;
	char *ptr = str;
	float *set = NULL;
	int sz = 0;
	while (ptr < str+strlen(str)) {
		float f = strtof(ptr, &ptr);
		set = realloc(set, ++sz * sizeof(float));
		set[sz-1] = f;
		while (*ptr &&
		       *ptr != '+' && *ptr != '-' && *ptr != '.' &&
		       !(*ptr > 0x2f && *ptr < 0x3a)) {
			ptr++;
		}
	}
	*s = sz;
	return set;
}

int main(int argc, char **argv) {
	if (argc < 5) {
		printf("Invalid arguments\n"
			"Usage: %s <input bmp> <output bmp> <input coords> <output coords>\n", argv[0]);
		return 0;
	}

	int inset_sz = 0, outset_sz = 0;
	float *inset = get_coordset(argv[3], &inset_sz);
	float *outset = get_coordset(argv[4], &outset_sz);
	if (!inset || !outset) return 1;

	int err = 0;
	if (inset_sz != outset_sz) {
		printf("input and output coord set sizes do not match (in: %d, out: %d)\n", inset_sz, outset_sz);
		err = 2;
	}
	if (inset_sz%2) {
		printf("coord sets share an odd size (%d)\n", inset_sz);
		err = 3;
	}
	if (inset_sz/2 < 3) {
		printf("not enough input/output coords (have: %d, need at least: 6)\n", inset_sz);
		err = 4;
	}
	if (inset_sz/2 > 4) {
		printf("too many input/output coords (have: %d, maximum: 8)\n", inset_sz);
		err = 5;
	}
	if (err) {
		free(inset);
		free(outset);
		return err;
	}

	tpolygon poly = {0};
	poly.n_vertices = inset_sz/2;
	int i;
	for (i = 0; i < poly.n_coords; i++) {
		poly.vertex[i].tx = inset[i*2];
		poly.vertex[i].ty = inset[i*2+1];
		poly.vertex[i].sx = outset[i*2];
		poly.vertex[i].sy = outset[i*2+1];
	}
	free(inset);
	free(outset);
	
	int x, y;
	tcolour *img = openbmp(argv[1], &x, &y);
	if (!img) return 6;

	poly.pix = img;
	poly.x = x;
	poly.y = y;

	int rx, ry;
	tcolour *render = trim_renderpolygon(&poly, &rx, &ry);
	free(img);
	if (!render) return 7;

	savebmp(argv[2], render, &rx, &ry);
	free(render);
	return 0;
}