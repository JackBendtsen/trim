#include "tkb.h"

void trim_update_old_input(void) {
	if (!trim_cur_kbst || !trim_cur_kbsize) return;

	trim_old_kbst = realloc(trim_old_kbst, trim_cur_kbsize * sizeof(tkey));
	trim_old_kbsize = trim_cur_kbsize;
	memcpy(trim_old_kbst, trim_cur_kbst, trim_old_kbsize);
}

#ifdef _WIN32_

void trim_initkb(int kb_mode) {
	if (kb_mode < TRIM_DEFKB || kb_mode > TRIM_RAWKB) return;
	trim_kb_mode = kb_mode;

	trim_old_kbst = NULL;
	trim_cur_kbst = NULL;
	trim_old_kbsize = 0;
	trim_cur_kbsize = 0;

	if (kb_mode == TRIM_DEFKB) {
		int kc[] = {
			
	}
}

#else /* ifdef _WIN32_ */

/*
In a Unix terminal application, there seem to be two main ways to read input in a manner suitable for non-printing.
  The simple way is to read raw data from stdin after configuring the TTY to turn off unwanted features.
The downside to this approach is that there seems to be no good way of determining if a key has been released,
which is essential for determining whether a key is held or not, which a lot of games rely on, especially for movement.
This approach should work on all Unix-based systems, however.
  The second approach is by reading data from the 'evdev' interface, which is currently only supported by Linux and the latest FreeBSD builds.
It does, however, allow for accurately deciding if a key is held or not by the event driven nature of the interface.
*/
void trim_initkb(int kb_mode) {
	if (kb_mode < TRIM_DEFKB || kb_mode > TRIM_RAWKB) return;
	trim_kb_mode = kb_mode;
	
	_trim_evfd = -1;
	trim_old_kbst = NULL;
	trim_cur_kbst = NULL;
	trim_old_kbsize = 0;
	trim_cur_kbsize = 0;

	char path[32];
	unsigned char bits[32];
	int idx = -1, i;
	struct stat st;

	// stdin configuration code taken from gcat.co.uk
	struct termios tty;

	/* make stdin non-blocking */
	int flags = fcntl(0, F_GETFL);
	flags |= O_NONBLOCK;
	fcntl(0, F_SETFL, flags);

	/* turn off buffering, echo and key processing */
	tcgetattr(0, &_tty_old);
	tty = _tty_old;
	tty.c_lflag &= ~(ICANON | ECHO | ISIG);
	tty.c_iflag &= ~(ISTRIP | INLCR | ICRNL | IGNCR | IXON | IXOFF);
	tcsetattr(0, TCSANOW, &tty);

	FD_ZERO(&_trim_fdset);
	FD_SET(0, &_trim_fdset);

	if (kb_mode == TRIM_DEFKB) return;

#ifdef __linux__
	// Attempt opening an input event device
	while (1) {
		idx++;
		sprintf(path, "/dev/input/event%d", idx);
		if (stat(path, &st) < 0) break;

		_trim_evfd = open(path, O_RDONLY);
		if (_trim_evfd < 0) continue;

		memset(bits, 0, 32);
		if (ioctl(_trim_evfd, EVIOCGBIT(0, sizeof(bits)), bits) < 0) {
			close(_trim_evfd);
			continue;
		}

		// Check if the first three bytes are 0x13, 0x00 and 0x12 respectively
		// If not, try another input device
		for (i = 0; i < 3; i++)
			if (bits[i] != "C0B"[i]-'0') break;

		if (i == 3) {
			_trim_kb_mode = TRIM_RAWKB;
			FD_ZERO(&_trim_fdset);
			FD_SET(_trim_evfd, &_trim_fdset);
			return;
		}
		close(_trim_evfd);
	}
#endif

	// If one is not found, the default method of reading input from stdin will be used
}

void trim_readinput(int *key, int wait) {
	trim_update_old_input();

	if (trim_kb_mode == TRIM_DEFKB) {
		int i;
		for (i = 0; i < trim_old_kbsize; i++) trim_old_kbst[i].state = 0;
		for (i = 0; i < trim_cur_kbsize; i++) trim_cur_kbst[i].state = 0;

		char buf[16] = {0};
		if (wait) select(1, &_trim_fdset, NULL, NULL, NULL);
		read(0, &buf[0], 15);

		int value = 0, sz = strlen(buf) < 4 ? strlen(buf) : 4;
		for (i = 0; i < sz; i++) value |= buf[sz-i-1] << (i*8);

		for (i = 0; i < trim_cur_kbsize; i++) {
			if (trim_cur_kbst[i].code == value) {
				trim_cur_kbst[i].state = 1;
				break;
			}
		}
		if (i == trim_cur_kbsize) {
			trim_cur_kbst = realloc(trim_cur_kbst, ++trim_cur_kbsize * sizeof(tkey));
			trim_cur_kbst[i].code = value;
			trim_cur_kbst[i].state = 1;
		}
		if (key) *key = i;
	}
	else {
#ifdef __linux__
		struct input_event ev[64];

		if (wait) select(1, &_trim_fdset, NULL, NULL, NULL);
		int r = read(_trim_evfd, ev, sizeof(ev));
		if (r < 0) {
			printf("Could not read from evdev (fd: %d, r: %d)\n", _trim_evfd, r);
			return;
		}

		int i, j, n_events = r / sizeof(struct input_event);
		for (i = 0; i < n_events; i++) {
			if (ev[i].type > 1) continue;
			for (j = 0; j < trim_cur_kbsize; j++) {
				if (trim_cur_kbst[j].code == ev[i].code) {
					trim_cur_kbst[j].state = ev[i].value;
					break;
				}
			}
			if (j == trim_cur_kbsize) {
				trim_cur_kbst = realloc(trim_cur_kbst, ++trim_cur_kbsize * sizeof(tkey));
				trim_cur_kbst[j].code = ev[i].code;
				trim_cur_kbst[j].state = ev[i].value;
			}
		}
		if (key) *key = j;
#endif
	}
}

// TODO: Flush evdev fd
void trim_closekb(void) {
	if (trim_old_kbst) free(trim_old_kbst);
	if (trim_cur_kbst) free(trim_cur_kbst);

	int flags = fcntl(0, F_GETFL);
	flags &= ~O_NONBLOCK;
	fcntl(0, F_SETFL, flags);
	tcsetattr(0, TCSANOW, &_tty_old);

	if (_trim_evfd > 0) close(_trim_evfd);
}

#endif /* ifndef _WIN32_ */

void trim_pollkb(void) {
	trim_readinput(NULL, 0);
}

int trim_getkey(void) {
	int key = 0;
	trim_readinput(&key, 1);
	if (trim_cur_kbst && key < trim_cur_kbsize) return trim_cur_kbst[key].code;
	return key;
}

int trim_keydown(int key) {
	int i;
	for (i = 0; i < trim_cur_kbsize; i++) {
		if (trim_cur_kbst[i].code == key && (i >= trim_old_kbsize ||
		    (trim_old_kbst[i].state == 0 && trim_cur_kbst[i].state == 1))) {
			return 1;
		}
	}
	return 0;
}

int trim_keyheld(int key) {
	int i;
	for (i = 0; i < trim_cur_kbsize; i++) {
		if (trim_cur_kbst[i].code == key && trim_cur_kbst[i].state == 1) {
			return 1;
		}
	}
	return 0;
}

int trim_keyup(int key) {
	int i;
	for (i = 0; i < trim_old_kbsize; i++) {
		if (trim_cur_kbst[i].code == key &&
		    trim_old_kbst[i].state == 1 &&
		    trim_cur_kbst[i].state == 0) {
			return 1;
		}
	}
	return 0;
}
