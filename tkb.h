#ifndef TKB_H
#define TKB_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32_
  #include <windows.h>
#else
  #include <unistd.h>
  #include <termios.h>
  #include <fcntl.h>
  #include <sys/types.h>
  #include <sys/stat.h>
  #include <sys/ioctl.h>
  #ifdef __linux__
    #include <linux/kd.h>
    #include <linux/input.h>
  #endif
  struct termios _trim_tty_old;
  int _trim_evfd;
  fd_set _trim_fdset;
#endif

#define TKEY_GRAVE    0
#define TKEY_1        1
#define TKEY_2        2
#define TKEY_3        3
#define TKEY_4        4
#define TKEY_5        5
#define TKEY_6        6
#define TKEY_7        7
#define TKEY_8        8
#define TKEY_9        9
#define TKEY_0       10
#define TKEY_MINUS   11
#define TKEY_EQUAL   12
#define TKEY_BKSP    13
#define TKEY_TAB     14
#define TKEY_Q       15
#define TKEY_W       16
#define TKEY_E       17
#define TKEY_R       18
#define TKEY_T       19
#define TKEY_Y       20
#define TKEY_U       21
#define TKEY_I       22
#define TKEY_O       23
#define TKEY_P       24
#define TKEY_LBRACE  25
#define TKEY_RBRACE  26
#define TKEY_BKSL    27
#define TKEY_CAPS    28
#define TKEY_A       29
#define TKEY_S       30
#define TKEY_D       31
#define TKEY_F       32
#define TKEY_G       33
#define TKEY_H       34
#define TKEY_J       35
#define TKEY_K       36
#define TKEY_L       37
#define TKEY_SMCOL   38
#define TKEY_APOS    39
#define TKEY_ENTER   40
#define TKEY_LSHIFT  41
#define TKEY_Z       42
#define TKEY_X       43
#define TKEY_C       44
#define TKEY_V       45
#define TKEY_B       46
#define TKEY_N       47
#define TKEY_M       48
#define TKEY_COMMA   49
#define TKEY_DOT     50
#define TKEY_FWSL    51
#define TKEY_RSHIFT  52
#define TKEY_LCTRL   53
#define TKEY_SYS1    54
#define TKEY_LALT    55
#define TKEY_SPACE   56
#define TKEY_RALT    57
#define TKEY_SYS2    58
#define TKEY_SYS3    59
#define TKEY_RCTRL   60

#define TKEY_ESC     61
#define TKEY_F1      62
#define TKEY_F2      63
#define TKEY_F3      64
#define TKEY_F4      65
#define TKEY_F5      66
#define TKEY_F6      67
#define TKEY_F7      68
#define TKEY_F8      69
#define TKEY_F9      70
#define TKEY_F10     71
#define TKEY_F11     72
#define TKEY_F12     73
#define TKEY_PRINT   74
#define TKEY_SCLOCK  75
#define TKEY_PAUSE   76
#define TKEY_INSERT  77
#define TKEY_HOME    78
#define TKEY_PGUP    79
#define TKEY_DEL     80
#define TKEY_END     81
#define TKEY_PGDOWN  82
#define TKEY_UP      83
#define TKEY_LEFT    84
#define TKEY_DOWN    85
#define TKEY_RIGHT   86

#define TKEY_NLOCK   87
#define TKEY_KPFWSL  88
#define TKEY_KPAST   89
#define TKEY_KPMIN   90
#define TKEY_KP7     91
#define TKEY_KP8     92
#define TKEY_KP9     93
#define TKEY_KPPLUS  94
#define TKEY_KP4     95
#define TKEY_KP5     96
#define TKEY_KP6     97
#define TKEY_KP1     98
#define TKEY_KP2     99
#define TKEY_KP3    100
#define TKEY_KPENT  101
#define TKEY_KP0    102
#define TKEY_KPDOT  103

#define TRIM_NKEYS  104

/*
#define TKEY_UP    0x1b5b41
#define TKEY_DOWN  0x1b5b42
#define TKEY_RIGHT 0x1b5b43
#define TKEY_LEFT  0x1b5b44
*/

#define TRIM_RAWKB 1
#define TRIM_DEFKB 0

int trim_kb_mode;

int trim_keycode[TRIM_NKEYS];

typedef struct {
	int idx;
	int state;
} tkey;

tkey *trim_old_kbst;
tkey *trim_cur_kbst;
int trim_old_kbsize;
int trim_cur_kbsize;

void trim_initkb(int kb_mode);
void trim_pollkb(void);
int trim_getkey(void);
int trim_keydown(int key);
int trim_keyheld(int key);
int trim_keyup(int key);
void trim_closekb(void);

#endif
