#include "historia.h"
#include "err.h"

void usun_wpis(wpis *wpisa)
{
	free(wpisa->wiadomosc);
}

size_t rozmiar_historii;

historia* historia_init(const size_t dlugosc_historii)
{
	historia *wynik;
	size_t i;
	wynik = malloc(sizeof(historia));
	if (wynik == NULL)
		syserr("Nie udało się zainicjalizować historii.");
	wynik->tablica_wpisow = malloc(sizeof(wpis *) * dlugosc_historii);
	if (wynik->tablica_wpisow == NULL) {
		syserr("Nie udało się zainicjalizować historii.");
	}
	for (i = 0; i < dlugosc_historii; ++i) {
		wynik->tablica_wpisow[i] = NULL;
	}
	rozmiar_historii = dlugosc_historii;
	wynik->glowa = -1;
	return wynik;
}

wpis* podaj_wpis(const historia *const hist, const size_t numer_wpisu)
{
	long long int tmp, tmp2;
	if (hist == NULL ||
	    hist->tablica_wpisow[hist->glowa] == NULL) {
		debug("Ktoś zażądał wpisu z pustej historii albo spod NULLA.");
		return NULL;
	}
	tmp = hist->glowa;
	tmp2 = numer_wpisu - tmp;
	while (tmp2 < 0)
		tmp2 += rozmiar_historii;
	tmp2 %= rozmiar_historii;
	if (hist->tablica_wpisow[tmp2] == NULL ||
	    hist->tablica_wpisow[tmp2]->numer_pakietu != numer_wpisu) {
		debug("Ktoś zażądał za starego wpisu.");
	}
	return hist->tablica_wpisow[tmp2];
}
