#include <stdio.h>
#include "../tkb.h"

int main() {
	trim_initkb();
	while (1) {
		int key = trim_poll();
		if (key) printf("%d\n", key);
		if (key == 'q') break;
	}
	trim_closekb();
	return 0;
}
