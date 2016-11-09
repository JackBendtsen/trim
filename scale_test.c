#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct scaler scaler;

struct scaler {
	int size;
	int len;
	double factor;

	int pos;
	int idx;
	double value;

	scaler *prev;
	scaler *next;

	void (*func)(void *dst, void *src, scaler *sd);
};

void scale_data(void *dst, void *src, scaler *sd) {
	if (!dst || !src || !sd || sd->factor == 0.0) return;

	double pos = 0.0, factor = sd->factor;
	int start = 0, end = sd->size, dir = 1;
	if (factor < 0.0) {
		start = sd->size-1;
		end = -1;
		dir = -1;
		factor = -factor;
	}

	int i;
	for (i = start; i != end; i += dir) {
		double span_left = factor;
		while (span_left > 0.0) {
			int p = (int)pos;
			double block = 1.0 - (pos - (double)p);
			double amount = span_left;
			if (block + 0.000001 < span_left) {
				amount = block;
				pos += amount;
				span_left -= amount;
			}
			else {
				pos += span_left;
				span_left = 0.0;
			}
			sd->pos = p;
			sd->idx = i;
			sd->value = amount;

			scale_data(dst, src, sd->next);
			if (sd->func) sd->func(dst, src, sd);
		}
	}
}
