#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <termios.h>
#include <fcntl.h>
#include <linux/input.h>
#include <sys/stat.h>

typedef unsigned char u8;

int open_kbd() {
	char path[32];
	u8 bits[32];
	int idx = -1, i, fd;
	struct stat st;

	while (1) {
		idx++;
		sprintf(path, "/dev/input/event%d", idx);
		if (stat(path, &st) < 0) {
			printf("Could not find %s\n", path);
			return -1;
		}

		fd = open(path, O_RDONLY);
		if (fd < 0) {
			printf("Could not open %s (%d)\n", path, fd);
			continue;
		}

		memset(bits, 0, 32);
		if (ioctl(fd, EVIOCGBIT(0, sizeof(bits)), bits) < 0) {
			printf("Could not read event bits from %s\n", path);
			close(fd);
			continue;
		}

		// Check if the first three bytes are 0x13, 0x00 and 0x12 respectively
		// If not, try another input device
		for (i = 0; i < 3; i++)
			if (bits[i] != "C0B"[i]-'0') break;

		if (i == 3) break;
		close(fd);
	}
	return fd;
}

int main() {
	struct termios tty, tty_old;

	/* make stdin non-blocking */
	int flags = fcntl(0, F_GETFL);
	flags |= O_NONBLOCK;
	fcntl(0, F_SETFL, flags);

	/* turn off buffering, echo and key processing */
	tcgetattr(0, &tty_old);
	tty = tty_old;
	tty.c_lflag &= ~(ICANON | ECHO | ISIG);
	tty.c_iflag &= ~(ISTRIP | INLCR | ICRNL | IGNCR | IXON | IXOFF);
	tcsetattr(0, TCSANOW, &tty);

	int kb_fd = open_kbd();
	if (kb_fd < 0) {
		printf("Could not open keyboard (%d)\n", kb_fd);
		return 1;
	}

	struct input_event ev[64];
	int quit = 0;
	while (!quit) {
		int r = read(kb_fd, ev, sizeof(ev));
		if (r < 0) {
			printf("Could not read from evdev (fd: %d, r: %d)\n", kb_fd, r);
			break;
		}

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

	flags = fcntl(0, F_GETFL);
	flags &= ~O_NONBLOCK;
	fcntl(0, F_SETFL, flags);
	tcsetattr(0, TCSANOW, &tty_old);

	close(kb_fd);
	return 0;
}
