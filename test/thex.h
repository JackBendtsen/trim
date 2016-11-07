#include "../tgfx.h"
#include "../tkb.h"

typedef unsigned long long int u64;

typedef struct {
	int type;
} action_t;

typedef struct {
	FILE *f;
	char *name;

	u64 off;
	u64 size;

	int n_digits;

	u8 *scr_buf;
	int scr_sz;

	int cur_x;
	int cur_y;

	int ctrl;
	int ro;

	u64 sel_off;
	u64 sel_sz;

	u8 *clip_buf;
	int clip_sz;

	action_t *act_stack;
	int act_ptr;
	int act_sz;

	u8 *search;
	int search_sz;
	u64 *results;
	int n_results;
} file_t;

typedef struct {
	int x, y;

	int len;
	int pos;

	char *name;

	char *str;
	int str_sz;
} field_t;

typedef struct {
	int x, y;
	int w, h;

	tcolour bg;

	field_t *fields;
	int n_fields;
	int field_idx;
} form_t;

file_t *open_file(char *ifname, char *ofname);
