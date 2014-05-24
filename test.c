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
	int ack, nr, win;
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
	free(napis);
	/* debug("Test czytania do vectora."); */
	/* fprintf(stderr, "wczytalismy %i\n", czytaj_do_vectora(STDIN_FILENO, &napis)); */
	/* fprintf(stderr, "wczytalismy:\n%s.\n", napis); */
	/* free(napis); */
	napis = malloc(50 * sizeof(char));
	/* debug("Test czytania do konca linii."); */
	/* fprintf(stderr, "wczytalismy %i\n", czytaj_do_konca_linii(STDIN_FILENO, napis, 50)); */
	/* fprintf(stderr, "wczytalismy:\n%s.\n", napis); */
	free(napis);
	/* debug("Test sendfile."); */
	/* debug("STDIN_FILENO = %i, STDOUT_FILENO = %i", STDIN_FILENO, STDOUT_FILENO); */
	/* fprintf(stderr, "%i\n", sendfile(STDOUT_FILENO, STDIN_FILENO, NULL, 15298)); */
	/* debug("A teraz kody błedów:\nEAGAIN=%i\nEBADF=%i\nEFAULT=%i\nEINVAL=%i\nEIO=%i\nENOMEM=%i", EAGAIN, EBADF, EFAULT, EINVAL, EIO, ENOMEM); */
	/* errno = 0; */
	/* debug("splice"); */
	/* fprintf(stderr, "%i\n", splice(STDIN_FILENO, NULL, STDOUT_FILENO, NULL, 5, 0)); */
	/* debug("po splice"); */
	assert(wyskub_dane_z_naglowka("ACK 5 3", &nr, &ack, &win) == 0);
	assert(ack == 5 && win == 3 && nr == -1);
	assert(wyskub_dane_z_naglowka("sghdgh", &nr, &ack, &win) != 0);
	assert(ack == -1 && nr == -1 && win  == -1);
	assert(wyskub_dane_z_naglowka("UPLOAD 30", &nr, &ack, &win) == 0);
	assert(nr == 30 && ack == -1 && win == -1);
	assert(wyskub_dane_z_naglowka("DATA 20 30 50", &nr, &ack, &win) == 0);
	assert(nr == 20 && ack == 30 && win == 50);
	assert(wyskub_dane_z_naglowka("DATA 5", &nr, &ack, &win) != 0);
	assert(ack == -1 && nr == -1 && win  == -1);
	assert(zrob_paczke_danych(5, 3, &napis) == 0);
	debug("%s.", napis);
	free(napis);
	printf("Przeszło wszystkie testy!\n");
	return EXIT_SUCCESS;
}
