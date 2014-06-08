/* Trzeci program zaliczeniowy z sieci komputerowych. */
/* Napisany przez Krzysztofa Piecucha, */
/* studenta informatyki k MISMaP UW */
/* numer albumu 332534 */
/* W projekcie zostały wykorzystane fragmenty kodu z zajęć. */
/* kodowanie UTF-8 */


#ifndef HISTORIA_H_HEDER
#define HISTORIA_H_HEDER
#include <stdlib.h>
#include <stddef.h>

typedef struct {
	size_t dlugosc_wiadomosci;
	void *wiadomosc;
	size_t numer_pakietu;
} wpis;

typedef struct {
	wpis **tablica_wpisow;
	size_t glowa;
} historia;

extern historia* historia_init(const int argc, char *const *const argv);

extern wpis* podaj_wpis(const historia *const hist, const size_t numer_wpisuj);

extern void dodaj_wpis(historia *const hist, const size_t numer_wpisu,
		       const void *const dane, const size_t rozmiar_danych);

#endif
