#ifndef TKB_H
#define TKB_H

#include <unistd.h>
#include <termios.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/kd.h>

#define TRIM_RAWKB 1
#define TRIM_DEFKB 0

int _trim_kb_mode;
int _trim_evfd;

typedef struct {
	int code;
	int state;
} tkey;

tkey *_trim_old_kbst;
tkey *_trim_cur_kbst;
int _trim_old_kbsize;
int _trim_cur_kbsize;

struct termios _tty_attr_old;

void trim_initkb(void);
int trim_poll(void);
int trim_keydown(int key);
int trim_keyheld(int key);
int trim_keyup(int key);
void trim_closekb(void);

#endif
