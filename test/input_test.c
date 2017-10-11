#include "../include/trim.h"

int main(int argc, char **argv) {
	TRIM_InitVideo(TGFX_RGB);
	TRIM_InitKB(TRIM_DEFKB);

	char str[4] = {0};
	TRIM_ApplyText(NULL, "Enter a key: ", 25, 10);
	TRIM_DrawScreen();
	while (1) {
		int key = TRIM_GetKey();
		sprintf(str, "%d", key);
		TRIM_ClearScreen();
		TRIM_ApplyText(NULL, "Enter a key: ", 25, 10);
		TRIM_ApplyText(NULL, str, 38, 10);
		TRIM_DrawScreen();
		if (key == TKEY_ESC) break;
	}

	TRIM_CloseVideo(1);
	TRIM_CloseKB();
	return 0;
}