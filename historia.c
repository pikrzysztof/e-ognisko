#include "historia.h"
#include "err.h"
#include "wspolne.h"
#include <string.h>
#include <stdio.h>

size_t rozmiar_historii;

static size_t podaj_rozmiar_historii(const int argc, char *const *const argv)
{
	const char *const OZNACZENIE = "-X";
	const char *const MAX = "200";
	const char *const MIN = "0";
	const char *const tmp = daj_opcje(OZNACZENIE, argc, argv);
	const size_t DOMYSLNIE = 20;
	size_t wynik;
	if (tmp == NULL)
		return DOMYSLNIE;
	if (jest_liczba_w_przedziale(MIN, MAX, tmp)) {
		if (sscanf(tmp, "%zu", &wynik) != 1) {
			syserr("Złe dane.");
		}
		return wynik;
	} else
		syserr("Opcja %s przyjmuje wartości od %s do %s.",
		       OZNACZENIE, MIN, MAX);
	return 0;
}

historia* historia_init(const int argc, char *const *const argv)
{
	historia *wynik;
	size_t i;
	wynik = malloc(sizeof(historia));
	if (wynik == NULL)
		syserr("Nie udało się zainicjalizować historii.");
	rozmiar_historii = podaj_rozmiar_historii(argc, argv);
	wynik->tablica_wpisow = malloc(sizeof(wpis *) * rozmiar_historii);
	if (wynik->tablica_wpisow == NULL) {
		syserr("Nie udało się zainicjalizować historii.");
	}
	for (i = 0; i < rozmiar_historii; ++i) {
		wynik->tablica_wpisow[i] = NULL;
	}
	wynik->glowa = SIZE_MAX;
	return wynik;
}

wpis* podaj_wpis(const historia *const hist, const size_t numer_wpisu)
{
	long long int tmp2;
	if (hist == NULL ||
	    hist->tablica_wpisow[hist->glowa] == NULL) {
		debug("Ktoś zażądał wpisu z pustej historii albo spod NULLA.");
		return NULL;
	}
	tmp2 = (long long int) numer_wpisu - (long long int) hist->glowa;
	while (tmp2 < 0)
		tmp2 += (long long int) rozmiar_historii;
	tmp2 %= (long long int) rozmiar_historii;
	if (hist->tablica_wpisow[tmp2] == NULL ||
	    hist->tablica_wpisow[tmp2]->numer_pakietu != numer_wpisu) {
		debug("Ktoś zażądał za starego wpisu.");
	}
	return hist->tablica_wpisow[tmp2];
}

static wpis *nowy_wpis(const size_t dlugosc_danych,
		       const void *const dane, const size_t nr_pakietu)
{
	wpis *wynik = malloc(sizeof(wpis));
	if (wynik == NULL)
		syserr("Mało pamięci.");
	wynik->wiadomosc = malloc(dlugosc_danych);
	if (wynik->wiadomosc == NULL)
		syserr("Mało pamięci.");
	memcpy(wynik->wiadomosc, dane, dlugosc_danych);
	wynik->numer_pakietu = nr_pakietu;
	wynik->dlugosc_wiadomosci = dlugosc_danych;
	return wynik;
}

static void usun_wpis(wpis *w)
{
	if (w != NULL)
		free(w->wiadomosc);
}

void dodaj_wpis(historia *const hist, const size_t numer_wpisu,
		const void *const dane, const size_t rozmiar_danych)
{
	++(hist->glowa);
	if (hist->glowa >= rozmiar_historii) {
		hist->glowa -= rozmiar_historii;
	}
	usun_wpis(hist->tablica_wpisow[hist->glowa]);
	free(hist->tablica_wpisow[hist->glowa]);
	hist->tablica_wpisow[hist->glowa] = nowy_wpis(rozmiar_danych, dane,
						      numer_wpisu);
}
