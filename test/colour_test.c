#include <stdio.h>

int main() {
	int i, j;
	for (i = 0; i < 16; i++) {
		for (j = 0; j < 16; j++) {
			printf("\x1b[48;5;%dm ", i*16+j);
		}
		printf("\x1b[0m\n");
	}
	return 0;
}