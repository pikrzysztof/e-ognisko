#include <string.h>
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
	char *napis = malloc(30 * sizeof(char));
	char *napis2 = "yyyy";
	#ifdef NDEBUG
	printf("Testy nie mają sensu, jest NDEBUG.\n");
	return EXIT_SUCCESS;
	#endif
	napis[0] = '0';
	napis[1] = '1';
	napis[2] = '2';
	napis[3] = '3';
	napis[4] = '4';
	napis[5] = ' ';
	napis[6] = '\0';
	assert(safe_add(INT16_MIN, INT16_MIN) == INT16_MIN);
	assert(safe_add(INT16_MAX, INT16_MAX) == INT16_MAX);
	assert(safe_add(INT16_MAX, 1) == INT16_MAX);
	assert(safe_add(INT16_MAX, INT16_MIN) == -1);
	assert(safe_add(INT16_MIN, -1) == INT16_MIN);
	printf("Przed konkatenacja napis 1:\n%s.\nnapis 2:\n%s.\n",
	       napis, napis2);
	konkatenacja(napis, napis2, strlen(napis2));
	printf("Po konkatenacji napis1:\n%s.\n", napis);
	dopisz_na_koncu(napis, "dopisane: %i", 5);
	printf("Dopisałem do napis1 na końcu \"dopisane: 5\n%s.\n", napis);
	assert(!jest_liczba_w_przedziale("0", "5", "10"));
	assert(jest_liczba_w_przedziale("0", "10", "5"));
	assert(jest_liczba_w_przedziale("50", "200", "100"));
	assert(jest_liczba_w_przedziale("1", "200", "50"));
	assert(jest_liczba_w_przedziale("100", "200", "103"));
	assert(jest_liczba_w_przedziale("100", "200", "199"));
	assert(!jest_liczba_w_przedziale("100", "200", "201"));
	assert(!jest_liczba_w_przedziale("1", "111", "999"));
	assert(!jest_liczba_w_przedziale("0", "10", "01"));
	assert(!jest_liczba_w_przedziale("aaaaa", "zzzzz", "ccccc"));
	assert(!jest_liczba_w_przedziale("aaaaa", "zzzzz", "cc"));
	return EXIT_SUCCESS;
}
