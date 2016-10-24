#include <stdio.h>
#include "tkb.h"

int main() {
	kb_init();
	while (1) {
		int key = kb_read();
		if (key) printf("%d\n", key);
		if (key == 'q') break;
	}
	kb_close();
	return 0;
}