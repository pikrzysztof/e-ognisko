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
#include <semaphore.h>
#include <inttypes.h>

/* Podaje false jak się nie uda operacja systemowa. */
bool V(sem_t *semafor)
{
	if (sem_post(semafor) != 0) {
		fprintf(stderr, "Nie udało się podnieść semafora.\n");
		return false;
	}
	return true;
}

/* Podaje false jak się nie uda operacja systemowa. */
bool P(sem_t *semafor)
{
	if (sem_wait(semafor) != 0) {
		fprintf(stderr, "Nie udało się opuścić semafora.\n");
		return false;
	}
	return true;
}

/* Wymaga uruchomienia pod semaforem. */
void ustaw_stan_FIFO(FIFO *const fifo)
{
	if (fifo->liczba_zuzytych_miejsc * sizeof(int16_t) <=
	    FIFO_LOW_WATERMARK)
		fifo->stan = FILLING;
	if (fifo->liczba_zuzytych_miejsc * sizeof(int16_t) >=
	    FIFO_HIGH_WATERMARK)
		fifo->stan = ACTIVE;
}

FIFO* zrob_FIFO()
{
	FIFO* wynik = malloc(sizeof(FIFO));

	if (wynik == NULL) {
		fprintf(stderr, "Nie dostaliśmy pamięci na kolejkę.");
		return NULL;
	}

	wynik->kolejka = malloc((FIFO_SIZE / sizeof(int16_t)) *
				  sizeof(int16_t));

	if (wynik->kolejka == NULL) {
		fprintf(stderr, "Nie dostaliśmy pamięci na kolejkę.");
		free(wynik);
		return NULL;
	}

	wynik->maksymalna_liczba_elementow = FIFO_SIZE / sizeof(int16_t);
	wynik->liczba_zuzytych_miejsc = 0;
	wynik->pierwszy = 0;
	/* 0, bo semafor nie jest dzielony miedzy watkami. */

	if (sem_init(wynik->mutex, 0, 1) != 0) {
		/* Nie udało się, czyścimy. */
		fprintf(stderr, "Nie udało się zainicjalizować semafora.\n");
		free(wynik->kolejka);
		free(wynik);
		return NULL;
	}

	if (sem_init(wynik->mozna_wlozyc, 0, FIFO_SIZE / 2) != 0) {
		fprintf(stderr, "Nie udało się zainicjalizować semafora.\n");
		if (sem_destroy(wynik->mutex) != 0)
			fprintf(stderr, "Nie udało się zwolnić semafora.\n");
		free(wynik->kolejka);
		free(wynik);
		return NULL;
	}

	return wynik;
}

int jest_pelna(FIFO *const fifo)
{
	int wynik;
	if (!P(fifo->mutex)) {
		/* Lepiej powiedzieć, że pełna niż ryzykować. */
		return -1;
	}
	fifo->liczba_zuzytych_miejsc == fifo->maksymalna_liczba_elementow ?
		wynik = 1 : wynik = 0;
	if (!V(fifo->mutex)) {
		/* Tutaj właściwie już będzie pogrom, bo nic nie zrobimy. */
		return -1;
	}
	return wynik;
}

int usun_FIFO(FIFO *const fifo)
{
	int wynik = 0;
	if (sem_destroy(fifo->mutex) != 0) {
		fprintf(stderr, "Nie udało się zniszczyć semafora.\n");
		wynik = -1;
	}
	if (sem_destroy(fifo->mozna_wlozyc) != 0) {
		fprintf(stderr, "Nie udało się zniszczyć semafora.\n");
		wynik = -1;
	}
	free(fifo->kolejka);
	free(fifo);
	return wynik;
}

int pop_FIFO(FIFO *const fifo)
{
	if (!P(fifo->mutex)) {
		return -1;
	}
	if (fifo->liczba_zuzytych_miejsc <= 0) {
		if (!V(fifo->mutex))
		    return -1;
		return 1;
	}
	--(fifo->liczba_zuzytych_miejsc);
	++(fifo->pierwszy);
	if (fifo->pierwszy == fifo->maksymalna_liczba_elementow) {
		fifo->pierwszy = 0;
	}
	if (!V(fifo->mozna_wlozyc)) {
		/* Nawet nie sprawdzam, i tak będzie -1 */
		V(fifo->mutex);
		return -1;
	}
	if (!V(fifo->mutex))
		return -1;
	return 0;
}

int push_FIFO(FIFO *const fifo, int16_t element)
{
	size_t miejsce_do_wrzucania;
	if (!P(fifo->mozna_wlozyc))
		return -1;
	if (!P(fifo->mutex))
		return -1;
	if (fifo->liczba_zuzytych_miejsc >= fifo->maksymalna_liczba_elementow) {
		if (!V(fifo->mutex))
			return -1;
		return 1;
	}
	++(fifo->liczba_zuzytych_miejsc);
	miejsce_do_wrzucania = fifo->liczba_zuzytych_miejsc + fifo->pierwszy;
	if (miejsce_do_wrzucania >= fifo->maksymalna_liczba_elementow) {
		miejsce_do_wrzucania -= fifo->maksymalna_liczba_elementow;
	}
	fifo->kolejka[miejsce_do_wrzucania] = element;
	if (!P(fifo->mutex)) {
		return -1;
	}
	return 0;
}

int16_t top_FIFO(FIFO *const fifo)
{
	int16_t wynik;
	if (!P(fifo->mutex)) {
		errno
		return -1;
	}
	if (fifo->liczba_zuzytych_miejsc == 0) {
		if (!V(fifo->mutex))
			return -1;
		return 0;
	}
	wynik = fifo->kolejka[fifo->pierwszy];
	if (!V(fifo->mutex))
		return -1;
	return wynik;
}

int wyczysc_FIFO(FIFO *const fifo)
{
	size_t i;
	if (!P(fifo->mutex)) {
		return -1;
	}
	for (i = 0; i < fifo->liczba_zuzytych_miejsc; i++) {
		if (!V(fifo->mozna_wlozyc))
			return -1;
	}
	fifo->pierwszy = 0;
	fifo->liczba_zuzytych_miejsc = 0;
	if (!V(fifo->mutex)) {
		return -1;
	}
	return 0;
}

size_t liczba_zuzytych_bajtow(const FIFO *const fifo)
{
	size_t wynik;
	if (!P(fifo->mutex))
		return SIZE_MAX;
	wynik = fifo->liczba_zuzytych_miejsc * sizeof(int16_t);
	if (!V(fifo->mutex))
		return SIZE_MAX;
	return wynik;
}
