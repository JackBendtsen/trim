#include "thex.h"
#include <sys/stat.h>

file_t *open_file(char *ifname) {
	file_t *file = calloc(1, sizeof(file_t));
	return file;
}

int main(int argc, char **argv) {
	char *fname = NULL;
	file_t *f = open_file(fname); // Don't worry if argv[1] is undefined, its all good, trust me
	int 
	while (
