/* Trzeci program zaliczeniowy z sieci komputerowych. */
/* Napisany przez Krzysztofa Piecucha, */
/* studenta informatyki k MISMaP UW */
/* numer albumu 332534 */
/* W projekcie zostały wykorzystane fragmenty kodu z zajęć. */
/* kodowanie UTF-8 */

#include "kolejka.h"
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <inttypes.h>

static bool jest_oznaczenie(const int argc, const char *const *const argv,
			    const char *const oznaczenie)
{
	int i;
	for (i = 0; i < argc; ++i) {
		if (strcmp(oznaczenie, i[argv]) == 0)
			return true;
	}
	return false;
}

int ustaw_wodnego_Marka(const int argc, const char *const *const argv)
{
	const char *const LOW_OZNACZENIE = "-L";
	const char *const HIGH_OZNACZENIE = "-H";
	const char *const MAX_ROZMIAR = "2147483647";
	const char *const MIN_ROZMIAR = "0";
	char *tmp;
	if (jest_oznaczenie(argc, argv, LOW_OZNACZENIE)) {
		tmp = daj_opcje(LOW_OZNACZENIE, argc, argv);
		if (tmp == NULL) {
			return -1;
		}
		if (!jest_liczba_w_przedziale(MIN_ROZMIAR, MAX_ROZMIAR, tmp)) {
			return -1;
		}
		FIFO_LOW_WATERMARK = atoi(tmp);
	}
	if (jest_oznaczenie(argc, argv, HIGH_OZNACZENIE)) {
		tmp = daj_opcje(HIGH_OZNACZENIE, argc, argv);
		if (tmp == NULL) {
			return -1;
		}
		if (!jest_liczba_w_przedziale(MIN_ROZMIAR, MAX_ROZMIAR, tmp)) {
			return -1;
		}
		FIFO_HIGH_WATERMARK = atoi(tmp);
	}
	if (FIFO_HIGH_WATERMARK <= FIFO_LOW_WATERMARK)
		return -1;
	if (FIFO_HIGH_WATERMARK > FIFO_SIZE)
		return -1;
	return 0;
}

int ustaw_rozmiar_fifo(int argc, const char *const *const argv)
{
	FIFO_SIZE = 10560;
	const char *const OZNACZENIE = "-F";
	const char *const MIN_ROZMIAR = "500";
	const char *const MAX_ROZMIAR = "500000";
	char tmp;
	if (jest_oznaczenie(argc, argv, OZNACZENIE)) {
		tmp = daj_opcje(argc, argv, OZNACZENIE);
		if (tmp == NULL)
			return -1;
		if (!jest_liczba_w_przedziale(MIN_ROZMIAR, MAX_ROZMIAR, tmp)) {
			return -1;
		}
		FIFO_SIZE = atoi(tmp);
	}
	return 0;
}
