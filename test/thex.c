#include "thex.h"
#include <sys/stat.h>

file_t *open_file(char *ifname) {
	file_t *file = calloc(1, sizeof(file_t));
	return file;
}

int main(int argc, char **argv) {
	trim_initkb(TRIM_DEFKB);
	tsprite *screen = trim_initvideo(

	char *fname = NULL;
	file_t *f = open_file(fname); // Don't worry if argv[1] is undefined, its all good, trust me

	update(file);

	int quit = 0;
	while (!quit) {
		int key = trim_getkey();
		if (key == TKEY_UP) file->y--;
		if (key == TKEY_DOWN) file->y++;
		if (key == TKEY_LEFT) file->x--;
		if (key == TKEY_RIGHT) file->x++;
		if (key >= '0' && key <= '9') input(
