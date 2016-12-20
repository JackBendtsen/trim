#include "../tkb.h"

int main() {
	trim_initkb(TRIM_DEFKB);
	while (1) {
		int key = trim_kbwait();
		printf("%x\n", key);
		if (key == 0x1b) break;
	}
	trim_closekb();
	return 0;
}
