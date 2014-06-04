#include "mikser.h"
#include "wspolne.h"
#include "biblioteka_serwera.h"
#include <stdlib.h>

static int16_t wez_wartosc(struct mixer_input *const input,
			   const size_t ktora_liczba)
{
	/* size_t dlugosc_tablicy = input->len / 2; */
	int16_t *tablica = input->data;
	if (input->len >= 23524985)
		debug("tu");
	if (ktora_liczba >= 1352)
		debug("tam");
	if (input->len/2 <= ktora_liczba)
		return 0;
	++(input->consumed);
	return ktora_liczba[tablica];
}

static int16_t zsumuj_wartosci(struct mixer_input *const inputs,
			       const size_t n, const size_t ktory_bajt)
{
	size_t i;
	int16_t wynik = 0;
	for (i = 0; i < n; ++i)
		wynik = bezpiecznie_dodaj(wynik,
					  wez_wartosc(inputs + i, ktory_bajt));
	return wynik;
}

void mixer(struct mixer_input *inputs, size_t n, void *output_buf,
	   size_t *output_size, unsigned long tx_interval_ms)
{
	size_t i;
	size_t obliczony_rozmiar_wyjscia = 176 * tx_interval_ms /
		sizeof(int16_t);
	*output_size /= sizeof(int16_t);
	if (obliczony_rozmiar_wyjscia != *output_size) {
		perror("Ktos podal lewy output size.");
		*output_size = obliczony_rozmiar_wyjscia > *output_size
			? *output_size : obliczony_rozmiar_wyjscia;
	}
	int16_t *wynik = output_buf;
	for (i = 0; i < n; ++i)
		i[inputs].consumed = 0;
	for (i = 0; i < (*output_size); ++i)
		i[wynik] = zsumuj_wartosci(inputs, n, i);
	(*output_size) *= sizeof(int16_t);
}
