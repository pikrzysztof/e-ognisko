#ifndef _MIKSER_H_
#define _MIKSER_H_
#include <stdlib.h>

struct mixer_input {
	void *data;
	size_t len;
	size_t consumed;
};

extern void mixer(struct mixer_input *inputs, size_t n,
		  void *output_buf, size_t *output_size,
		  unsigned long tx_interval_ms);


#endif
