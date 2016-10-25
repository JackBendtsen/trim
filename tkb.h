#ifndef TKB_H
#define TKB_H

#include <unistd.h>
#include <termios.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/kd.h>

struct termios tty_attr_old;
int old_keyboard_mode;

void kb_init(void);
int kb_read(void);
void kb_close(void);

#endif
