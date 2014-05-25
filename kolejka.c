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
#include "wspolne.h"
#include "err.h"
static bool jest_oznaczenie(const int argc, char *const *const argv,
			    const char *const oznaczenie)
{
	int i;
	for (i = 0; i < argc; ++i) {
		if (strcmp(oznaczenie, i[argv]) == 0)
			return true;
	}
	return false;
}

void ustaw_wodnego_Marka(const int argc, char *const *const argv)
{
	const char *const LOW_OZNACZENIE = "-L";
	const char *const HIGH_OZNACZENIE = "-H";
	const char *const MAX_ROZMIAR = "2147483647";
	const char *const MIN_ROZMIAR = "0";
	const size_t DOMYSLNY_NISKI_MAREK =  0;
	const char *tmp;
	if (jest_oznaczenie(argc, argv, LOW_OZNACZENIE)) {
		tmp = daj_opcje(LOW_OZNACZENIE, argc, argv);
		if (tmp == NULL)
			fatal("Zle podany parametr %s.", LOW_OZNACZENIE);
		if (!jest_liczba_w_przedziale(MIN_ROZMIAR, MAX_ROZMIAR, tmp))
			fatal("Zle podany parametr %s.", LOW_OZNACZENIE);
		FIFO_LOW_WATERMARK = atoi(tmp);
	} else {
		FIFO_LOW_WATERMARK = DOMYSLNY_NISKI_MAREK;
	}
	if (jest_oznaczenie(argc, argv, HIGH_OZNACZENIE)) {
		tmp = daj_opcje(HIGH_OZNACZENIE, argc, argv);
		if (tmp == NULL)
			fatal("Zle podany parametr %s.", HIGH_OZNACZENIE);
		if (!jest_liczba_w_przedziale(MIN_ROZMIAR, MAX_ROZMIAR, tmp))
			fatal("Zle podany parametr %s.", HIGH_OZNACZENIE);
		FIFO_HIGH_WATERMARK = atoi(tmp);
	} else {
		FIFO_HIGH_WATERMARK = FIFO_SIZE;
	}
	if (FIFO_HIGH_WATERMARK <= FIFO_LOW_WATERMARK)
		fatal("Zle podane watermarki.");
	if (FIFO_HIGH_WATERMARK > FIFO_SIZE)
		fatal("Zle podane watermarki.");
}

void ustaw_rozmiar_fifo(int argc, char *const *const argv)
{
	FIFO_SIZE = 10560;
	const char *const OZNACZENIE = "-F";
	const char *const MIN_ROZMIAR = "500";
	const char *const MAX_ROZMIAR = "500000";
	const char *tmp;
	if (jest_oznaczenie(argc, argv, OZNACZENIE)) {
		tmp = daj_opcje(OZNACZENIE, argc, argv);
		if (tmp == NULL)
			fatal("Parametr %s zle podany.", OZNACZENIE);
		if (!jest_liczba_w_przedziale(MIN_ROZMIAR, MAX_ROZMIAR, tmp))
			fatal("Parametr %s zle podany.", OZNACZENIE);
		FIFO_SIZE = atoi(tmp);
	}
}
