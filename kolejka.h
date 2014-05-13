#ifndef _KOLEJKA_H_
#define _KOLEJKA_H_

#include <inttypes.h>
#include <semaphore.h>

/* Funkcje podają 0 jeśli ich wywołanie się powiodło, */
/* -1, jeśli napotkały na bład systemowy, */
/* 1, jeśli był inny bład. */
/* Wszystkie funkcje w razie podania -1 zapewniają, że kolejkę można usunąć. */

enum stan_FIFO {
	ACTIVE,
	FILLING
};

typedef struct {
	sem_t *mutex; 		/* Tylko jeden watek ma dostep do kolejki. */
	sem_t *mozna_wlozyc;	/* Do czekania kiedy kolejka jest pełna. */
	enum stan_FIFO stan;
	size_t liczba_zuzytych_miejsc;  /* Stopien zapelniena kolejki. */
	size_t maksymalna_liczba_elementow;
	size_t pierwszy;        /* Wskazuje na pierwszy element kolejki. */
	int16_t *kolejka;
} FIFO;

size_t FIFO_SIZE /* = 10560 */;
size_t FIFO_LOW_WATERMARK /* = 0 */;
size_t FIFO_HIGH_WATERMARK /* = FIFO_SIZE */;

/* Atomowe. Daje NULL jeśli się nie powiedzie. */
extern FIFO* zrob_FIFO();

/* Atomowe. Daje -1 jeśli się nie powiedzie. */
extern int usun_FIFO(FIFO *const fifo);

extern int pop_FIFO(FIFO *const fifo);

extern int push_FIFO(FIFO *const fifo, int16_t element);

extern int16_t top_FIFO(FIFO *const fifo);

extern int wyczysc_FIFO(FIFO *const fifo);

extern int jest_pelna(FIFO *const fifo);

#endif
