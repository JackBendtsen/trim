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

int ps(void *ptr1, void *ptr2) {
	return ptr2-ptr1;
}

void print_bmp_t(bmp_t *h) {
	printf("sizeof(bmp_t): %d\n\n"
		"%d - file_sz: %d\n"
		"%d - un1: %d\n"
		"%d - un2: %d\n"
		"%d - off: %d\n"
		"%d - dib_sz: %d\n"
		"%d - x: %d\n"
		"%d - y: %d\n"
		"%d - ndp: %d\n"
		"%d - bpp: %d\n"
		"%d - comp: %d\n"
		"%d - data_sz: %d\n"
		"%d - x_res: %d\n"
		"%d - y_res: %d\n"
		"%d - nc: %d\n"
		"%d - nic: %d\n", sizeof(bmp_t),
		ps(h, &h->file_sz), h->file_sz,
		ps(h, &h->un1), h->un1,
		ps(h, &h->un2), h->un2,
		ps(h, &h->off), h->off,
		ps(h, &h->dib_sz), h->dib_sz,
		ps(h, &h->x), h->x,
		ps(h, &h->y), h->y,
		ps(h, &h->ndp), h->ndp,
		ps(h, &h->bpp), h->bpp,
		ps(h, &h->comp), h->comp,
		ps(h, &h->data_sz), h->data_sz,
		ps(h, &h->x_res), h->x_res,
		ps(h, &h->y_res), h->y_res,
		ps(h, &h->nc), h->nc,
		ps(h, &h->nic), h->nic);
}

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
	print_bmp_t(&hdr);

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

void savebmp(char *name, tcolour *img, int w, int h) {
	if (!name || !img) return;

	int sz = w*h*4;
	char *magic = "BM";
	bmp_t hdr = {0};

	hdr.file_sz = sz + 54;
	hdr.off = 54;
	hdr.dib_sz = 40;
	hdr.x = w;
	hdr.y = h;
	hdr.ndp = 1;
	hdr.bpp = 32;
	hdr.data_sz = sz;

	FILE *f = fopen(name, "wb");
	if (!f) return;
	fwrite(magic, 1, 2, f);
	fwrite(&hdr, sizeof(bmp_t), 1, f);

	u8 *out = malloc(sz);
	int i, j, p = 0;
	for (i = h-1; i >= 0; i--) {
		for (j = 0; j < w; j++, p += 4) {
			out[p] = img[i*w + j].b;
			out[p+1] = img[i*w + j].g;
			out[p+2] = img[i*w + j].r;
			out[p+3] = img[i*w + j].a;
		}
	}

	fwrite(out, 1, sz, f);
	fclose(f);
	free(out);
}
/*
tcolour *trim_renderpolygon(tpolygon *p, int *rx, int *ry) {
	if (!p || !rx || !ry) return NULL;

	if (p->n_vertices < 3 || p->n_vertices > 4) {
		printf("Error: invalid number of vertices (%d)\n", p->n_vertices);
		return NULL;
	}

	int i;
	for (i = 0; i 
*/
float *get_coordset(char *str, int *s) {
	if (!str || !s) return NULL;
	char *ptr = str;
	float *set = NULL;
	int sz = 0;
	while (ptr < str+strlen(str)) {
		float f = strtod(ptr, &ptr);
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
	for (i = 0; i < poly.n_vertices; i++) {
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
/*
	poly.pix = img;
	poly.x = x;
	poly.y = y;

	int rx, ry;
	tcolour *render = trim_renderpolygon(&poly, &rx, &ry);
	free(img);
	if (!render) return 7;
*/
	savebmp(argv[2], img, x, y);
	free(img);
	return 0;
}
