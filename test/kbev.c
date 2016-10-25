#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <linux/input.h>

typedef unsigned char u8;

int open_kbd() {
	FILE *f = fopen("/proc/bus/input/devices", "r");
	if (!f) return -1;

	fseek(f, 0, SEEK_END);
	int sz = ftell(f);
	rewind(f);

	char *buf = malloc(sz);
	fread(buf, 1, sz, f);
	fclose(f);

	printf("buf: %p, sz: %d\n", buf, sz);

	char *p = strstr(buf, "keyboard");
	if (!p) {
		free(buf);
		return -2;
	}

	p = strstr(p, "event");
	if (!p) {
		free(buf);
		return -3;
	}

	char path[32] = {0};
	strcpy(path, "/dev/input/");
	memcpy(path+strlen(path), p, strcspn(p, " \n"));
	free(buf);
	return open(path, O_RDONLY);
}

int main() {
	int kb_fd = open("/dev/input/event3", O_RDONLY);
	if (kb_fd < 0) {
		printf("Could not open keyboard (%d)\n", kb_fd);
		return 1;
	}

	struct input_event ev[64];
	int quit = 0;
	while (!quit) {
		int r = read(kb_fd, ev, sizeof(ev));
		if (r < 0) break;

		int n_events = r / sizeof(struct input_event);
		if (n_events < 1) {
			continue;
		}

		int i;
		for (i = 0; i < n_events; i++) {
			if (ev[i].type == 1 && ev[i].value != 2) printf("Type: %d, Code: %d, Value: %d\n", ev[i].type, ev[i].code, ev[i].value);
			if (ev[i].code == 16) quit = 1;
		}
	}

	close(kb_fd);
	return 0;
}
