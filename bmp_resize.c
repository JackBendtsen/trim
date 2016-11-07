#include "scale_test.c"

typedef unsigned char u8;

typedef struct {
	u8 r, g, b, a;
} tcolour;

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

void savebmp(char *name, tcolour *img, int w, int h) {
	if (!name || !img) return;
	if (w < 0) w = -w;
	if (h < 0) h = -h;

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

void resize_pixel(void *dst, void *src, scaler *sd) {
	double *d = (double*)dst;
	tcolour *s = (tcolour*)src;

	// sd contains info about x whereas sd->prev contains info about y
	double area = sd->value * sd->prev->value;

	int s_idx = sd->prev->idx * sd->size + sd->idx;

	int d_width = sd->size * sd->factor;
	if (d_width < 0) d_width = -d_width;
	int d_idx = sd->prev->pos * d_width + sd->pos;

	u8 *s_pix = (u8*)(&s[s_idx]);

	int i;
	for (i = 0; i < 4; i++) {
		double f = d[d_idx*4 + i] + (double)s_pix[i] * area;
		if (f > 255.0) f = 255.0;
		if (f < 0.0) f = 0.0;
		d[d_idx*4 + i] = f;
	}
}

int main(int argc, char **argv) {
	if (argc < 5) {
		printf("Invalid arguments\n"
			"Usage: %s <input bmp> <output bmp> <new width> <new height>\n", argv[0]);
		return 0;
	}

	int w = atoi(argv[3]);
	int h = atoi(argv[4]);

	int x, y;
	tcolour *img = openbmp(argv[1], &x, &y);
	if (!img) return 6;

	scaler w_rs = {0};
	scaler h_rs = {0};

	w_rs.size = x;
	w_rs.factor = (double)w / (double)x;
	w_rs.func = resize_pixel;
	w_rs.prev = &h_rs;

	h_rs.size = y;
	h_rs.factor = (double)h / (double)y;
	h_rs.next = &w_rs;

	if (w < 0) w = -w;
	if (h < 0) h = -h;

	float *frs = calloc(w * h * 4, sizeof(double));
	scale_data(frs, img, &h_rs);

	tcolour *rs = calloc(w * h, sizeof(tcolour));
	int i, j;
	for (i = 0; i < w*h; i++) {
		u8 *ptr = (u8*)&rs[i];
		for (j = 0; j < 4; j++) ptr[j] = (u8)frs[i*4+j];
	}
	free(frs);

	savebmp(argv[2], rs, w, h);
	free(img);
	return 0;
}
