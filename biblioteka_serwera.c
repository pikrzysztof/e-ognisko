#include "biblioteka_serwera.h"
#include <inttypes.h>

/* Jeśli miałoby nastąpić przepełnienie daje wartość maksymalną/minimalną typu. */
int16_t safe_add(const int16_t first, const int16_t second)
{
	const int16_t result = first + second;
	if (first < 0 && second < 0 && (result > first || result > second))
		return INT16_MIN;
	if (first > 0 && second > 0 && (result < first || result < second))
		return INT16_MAX;
	return result;
}
