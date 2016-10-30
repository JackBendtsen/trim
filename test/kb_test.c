#include "../tkb.h"

int print_keys() {
	int i, t = 0, q = 0;
	for (i = 0; i < _trim_cur_kbsize; i++) {
		if (i >= _trim_old_kbsize || _trim_cur_kbst[i].state == 1) {
			printf("%x ", _trim_cur_kbst[i].code);
			if (_trim_cur_kbst[i].code == 'q') q = 1;
			t = 1;
		}
	}
	if (t) putchar('\n');
	return q;
}

int main() {
	trim_initkb();
	int quit = 0;
	while (!quit) {
		trim_poll();
		quit = print_keys();
	}
	trim_closekb();
	return 0;
}
