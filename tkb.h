#ifndef TKB_H
#define TKB_H

#include <unistd.h>
#include <termios.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef __linux__
  #include <linux/kd.h>
  #include <linux/input.h>
#endif

#define TKEY_UP    0x1b5b41
#define TKEY_DOWN  0x1b5b42
#define TKEY_RIGHT 0x1b5b43
#define TKEY_LEFT  0x1b5b44

#define TRIM_RAWKB 1
#define TRIM_DEFKB 0

int trim_kb_mode;

int _trim_evfd;
fd_set _trim_fdset;

typedef struct {
	int code;
	int state;
} tkey;

tkey *trim_old_kbst;
tkey *trim_cur_kbst;
int trim_old_kbsize;
int trim_cur_kbsize;

struct termios _tty_old;

void trim_initkb(int kb_mode);
void trim_pollkb(void);
int trim_getkey(void);
int trim_keydown(int key);
int trim_keyheld(int key);
int trim_keyup(int key);
void trim_closekb(void);

#endif
