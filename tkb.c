#include "tkb.h"

void kb_init(void) {
	struct termios tty_attr;

	/* make stdin non-blocking */
	int flags = fcntl(0, F_GETFL);
	flags |= O_NONBLOCK;
	fcntl(0, F_SETFL, flags);

	ioctl(0, KDGKBMODE, &old_keyboard_mode);
	tcgetattr(0, &tty_attr_old);

	/* turn off buffering, echo and key processing */
	tty_attr = tty_attr_old;
	tty_attr.c_lflag &= ~(ICANON | ECHO | ISIG);
	tty_attr.c_iflag &= ~(ISTRIP | INLCR | ICRNL | IGNCR | IXON | IXOFF);
	tcsetattr(0, TCSANOW, &tty_attr);

	ioctl(0, KDSKBMODE, K_RAW);
}

int kb_read(void) {
	char buf[1];
	int res;

	res = read(0, &buf[0], 1);
	if (res < 0) return 0;
	return buf[0];
}

void kb_close(void) {
	int flags = fcntl(0, F_GETFL);
	flags &= ~O_NONBLOCK;
	fcntl(0, F_SETFL, flags);
	tcsetattr(0, TCSANOW, &tty_attr_old);
	ioctl(0, KDSKBMODE, old_keyboard_mode);
}
