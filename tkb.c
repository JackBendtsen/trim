#include "tkb.h"

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
	_trim_kb_mode = TRIM_DEFKB;
	_trim_evfd = -1;
	_trim_old_kbst = NULL;
	_trim_cur_kbst = NULL;
	_trim_old_kbsize = 0;
	_trim_cur_kbsize = 0;

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
			return /*TRIM_RAWKB*/;
		}
		close(_trim_evfd);
	}
#endif

	// If one is not found, the default method of reading input from stdin will be used
	//return TRIM_DEFKB;
}

void trim_readinput(int *key, int wait) {
	if (_trim_cur_kbst && _trim_cur_kbsize) {
		_trim_old_kbst = realloc(_trim_old_kbst, _trim_cur_kbsize * sizeof(tkey));
		_trim_old_kbsize = _trim_cur_kbsize;
		memcpy(_trim_old_kbst, _trim_cur_kbst, _trim_old_kbsize);
	}
	if (_trim_kb_mode == TRIM_DEFKB) {
		int i;
		for (i = 0; i < _trim_old_kbsize; i++) _trim_old_kbst[i].state = 0;
		for (i = 0; i < _trim_cur_kbsize; i++) _trim_cur_kbst[i].state = 0;

		char buf[16] = {0};
		if (wait) select(1, &_trim_fdset, NULL, NULL, NULL);
		read(0, &buf[0], 15);

		int value = 0, sz = strlen(buf) < 4 ? strlen(buf) : 4;
		for (i = 0; i < sz; i++) value |= buf[sz-i-1] << (i*8);

		for (i = 0; i < _trim_cur_kbsize; i++) {
			if (_trim_cur_kbst[i].code == value) {
				_trim_cur_kbst[i].state = 1;
				break;
			}
		}
		if (i == _trim_cur_kbsize) {
			_trim_cur_kbst = realloc(_trim_cur_kbst, ++_trim_cur_kbsize * sizeof(tkey));
			_trim_cur_kbst[i].code = value;
			_trim_cur_kbst[i].state = 1;
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
			for (j = 0; j < _trim_cur_kbsize; j++) {
				if (_trim_cur_kbst[j].code == ev[i].code) {
					_trim_cur_kbst[j].state = ev[i].value;
					break;
				}
			}
			if (j == _trim_cur_kbsize) {
				_trim_cur_kbst = realloc(_trim_cur_kbst, ++_trim_cur_kbsize * sizeof(tkey));
				_trim_cur_kbst[j].code = ev[i].code;
				_trim_cur_kbst[j].state = ev[i].value;
			}
		}
		if (key) *key = j;
#endif
	}
}

void trim_pollkb(void) {
	trim_readinput(NULL, 0);
}

int trim_getkey(void) {
	int key = 0;
	trim_readinput(&key, 1);
	if (_trim_cur_kbst && key < _trim_cur_kbsize) return _trim_cur_kbst[key].code;
	return key;
}

int trim_keydown(int key) {
	int i;
	for (i = 0; i < _trim_cur_kbsize; i++) {
		if (_trim_cur_kbst[i].code == key && (i >= _trim_old_kbsize ||
		    (_trim_old_kbst[i].state == 0 && _trim_cur_kbst[i].state == 1))) {
			return 1;
		}
	}
	return 0;
}

int trim_keyheld(int key) {
	int i;
	for (i = 0; i < _trim_cur_kbsize; i++) {
		if (_trim_cur_kbst[i].code == key && _trim_cur_kbst[i].state == 1) {
			return 1;
		}
	}
	return 0;
}

int trim_keyup(int key) {
	int i;
	for (i = 0; i < _trim_old_kbsize; i++) {
		if (_trim_cur_kbst[i].code == key &&
		    _trim_old_kbst[i].state == 1 &&
		    _trim_cur_kbst[i].state == 0) {
			return 1;
		}
	}
	return 0;
}

// TODO: Flush evdev fd
void trim_closekb(void) {
	if (_trim_old_kbst) free(_trim_old_kbst);
	if (_trim_cur_kbst) free(_trim_cur_kbst);

	int flags = fcntl(0, F_GETFL);
	flags &= ~O_NONBLOCK;
	fcntl(0, F_SETFL, flags);
	tcsetattr(0, TCSANOW, &_tty_old);

	if (_trim_evfd > 0) close(_trim_evfd);
}
