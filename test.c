#include <fcntl.h>
#include <string.h>
#include <sys/sendfile.h>
#include "biblioteka_serwera.h"
#include "kolejka.h"
#include "wspolne.h"
#include "err.h"
#include <inttypes.h>
#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>

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
	konkatenacja(napis, napis2, strlen(napis2) + 1);
	assert(strcmp(napis, "01234 yyyy") == 0);
	dopisz_na_koncu(napis, "dopisane: %i", 5);
	assert(strcmp(napis, "01234 yyyydopisane: 5") == 0);
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
	printf("Przeszło wszystkie testy!\n");
	free(napis);
	debug("Test czytania do vectora.");
	fprintf(stderr, "wczytalismy %i\n", czytaj_do_vectora(STDIN_FILENO, &napis));
	fprintf(stderr, "wczytalismy:\n%s.\n", napis);
	free(napis);
	napis = malloc(50 * sizeof(char));
	debug("Test czytania do konca linii.");
	fprintf(stderr, "wczytalismy %i\n", czytaj_do_konca_linii(STDIN_FILENO, napis, 50));
	fprintf(stderr, "wczytalismy:\n%s.\n", napis);
	free(napis);
	debug("Test sendfile.");
	debug("STDIN_FILENO = %i, STDOUT_FILENO = %i", STDIN_FILENO, STDOUT_FILENO);
	fprintf(stderr, "%i\n", sendfile(STDOUT_FILENO, STDIN_FILENO, NULL, 15298));
	debug("A teraz kody błedów:\nEAGAIN=%i\nEBADF=%i\nEFAULT=%i\nEINVAL=%i\nEIO=%i\nENOMEM=%i", EAGAIN, EBADF, EFAULT, EINVAL, EIO, ENOMEM);
	errno = 0;
	debug("splice");
	fprintf(stderr, "%i\n", splice(STDIN_FILENO, NULL, STDOUT_FILENO, NULL, 5, 0));
	debug("po splice");
	return EXIT_SUCCESS;
}
