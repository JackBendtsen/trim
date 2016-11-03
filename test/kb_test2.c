#include "../tkb.h"

int main() {
	trim_initkb(TRIM_DEFKB);
	printf("| |");
	fflush(stdout);

	int ssz = 1;
	char *str = malloc(ssz);
	str[0] = ' ';

	int i, p = 0, update = 0;
	while (1) {
		trim_kbwait();
		if (trim_keydown(0x1b)) break;
		if (trim_keydown(TKEY_UP)) {
			str[p]++;
			if (str[p] == 0x7f) str[p] = 0x20;
			update = 1;
		}
		else if (trim_keydown(TKEY_DOWN)) {
			str[p]--;
			if (str[p] == 0x1f) str[p] = 0x7e;
			update = 1;
		}
		else if (trim_keydown(TKEY_LEFT)) {
			p--;
			if (p < 0) p = 0;
			update = 1;
		}
		else if (trim_keydown(TKEY_RIGHT)) {
			p++;
			if (p >= ssz) {
				if (p == 97) p = 96;
				else {
					ssz = p+1;
					str = realloc(str, ssz);
					str[p] = ' ';
				}
			}
			update = 1;
		}
		if (update) {
			for (i = 0; i < ssz+2; i++) putchar('\b');
			putchar('|');
			for (i = 0; i < ssz; i++) {
				if (i == p) printf("\x1b[30;47m");
				putchar(str[i]);
				if (i == p) printf("\x1b[0m");
			}
			putchar('|');
			fflush(stdout);
			update = 0;
		}
	}
	putchar('\n');
	trim_closekb();
	return 0;
}
