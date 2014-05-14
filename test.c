#include "biblioteka_serwera.h"
#include "kolejka.h"
#include "wspolne.h"
#include "err.h"
#include <inttypes.h>
#include <assert.h>
#include <stdlib.h>
#include <stdio.h>

int main()
{
	assert(safe_add(INT16_MIN, INT16_MIN) == INT16_MIN);
	assert(safe_add(INT16_MAX, INT16_MAX) == INT16_MAX);
	assert(safe_add(INT16_MAX, 1) == INT16_MAX);
	assert(safe_add(INT16_MAX, INT16_MIN) == -1);
	assert(safe_add(INT16_MIN, -1) == INT16_MIN);
	return EXIT_SUCCESS;
}
