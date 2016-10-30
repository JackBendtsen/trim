#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, char **argv) {
	if (argc != 4) {
		printf("Invalid arguments\n"
			"Usage: %s <input bmp> <output bmp> <input coords> <output coords>\n", argv[0]);
		return 0;
	}