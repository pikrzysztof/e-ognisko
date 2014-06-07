/* Trzeci program zaliczeniowy z sieci komputerowych. */
/* Napisany przez Krzysztofa Piecucha, */
/* studenta informatyki k MISMaP UW */
/* numer albumu 332534 */
/* W projekcie zostały wykorzystane fragmenty kodu z zajęć. */
/* kodowanie UTF-8 */

#include "kolejka.h"
#include <unistd.h>
#include "klient_struct.h"
#include "mikser.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <inttypes.h>
#include "wspolne.h"
#include "err.h"
#include <assert.h>

size_t FIFO_SIZE;
size_t FIFO_LOW_WATERMARK;
size_t FIFO_HIGH_WATERMARK;

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

void init_wodnego_Marka(const int argc, char *const *const argv)
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

void usun_FIFO(FIFO *const fifo)
{
	free(fifo->kolejka);
	free(fifo);
}

FIFO* init_FIFO()
{
	FIFO *wynik = malloc(sizeof(FIFO));
	if (wynik == NULL)
		return NULL;
	wynik->kolejka = malloc(FIFO_SIZE);
	if (wynik->kolejka == NULL) {
		free(wynik);
		return NULL;
	}
	wynik->liczba_zuzytych_bajtow = 0;
	wynik->stan = FILLING;
	memset(wynik->kolejka, 0, FIFO_SIZE);
	return wynik;
}

void ustaw_wodnego_Marka(FIFO *const fifo)
{
	if (fifo->liczba_zuzytych_bajtow <= FIFO_LOW_WATERMARK)
		fifo->stan = FILLING;
	if (fifo->liczba_zuzytych_bajtow >= FIFO_HIGH_WATERMARK)
		fifo->stan = ACTIVE;
}

size_t daj_FIFO_SIZE()
{
	return FIFO_SIZE;
}

int dodaj(FIFO *const fifo, void *dane, size_t rozmiar_danych)
{
	char *tmp = dane;
	tmp = strchr(dane, '\n');
	++tmp;
	rozmiar_danych -= tmp - (char *)dane;
	if (daj_FIFO_SIZE() - (fifo->liczba_zuzytych_bajtow) < rozmiar_danych) {
		debug("za dużo danych");
		return -1;
	}
	memcpy(((char *) (fifo->kolejka)) + fifo->liczba_zuzytych_bajtow,
	       tmp, rozmiar_danych);
	fifo->liczba_zuzytych_bajtow += rozmiar_danych;
	ustaw_wodnego_Marka(fifo);
	return 0;
}

int32_t daj_win(FIFO *fifo)
{
	return FIFO_SIZE - fifo->liczba_zuzytych_bajtow;
}

static usun_poczatek(FIFO *const fifo, const size_t ile)
{
	assert(fifo->liczba_zuzytych_bajtow >= ile);
	fifo->liczba_zuzytych_bajtow -= ile;
	memmove(fifo->kolejka, fifo->kolejka + ile,
		fifo->liczba_zuzytych_bajtow);
	ustaw_wodnego_Marka(fifo);
}


void odejmij_ludziom(klient **klienci, struct mixer_input *inputs,
		     const size_t MAX_KLIENTOW)
{
	size_t i, aktywni;
	for (i = 0, aktywni = 0; i < MAX_KLIENTOW; ++i) {
		if (klienci[i] == NULL ||
		    !klienci[i]->potwierdzil_numer ||
		    klienci[i]->kolejka->stan != ACTIVE) {
			continue;
		}
		usun_poczatek(klienci[i]->kolejka,
			      inputs[aktywni].consumed);
		++aktywni;
		uaktualnij_wartosci_sitrepu(klienci[i]);
	}
}
