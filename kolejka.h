/* Trzeci program zaliczeniowy z sieci komputerowych. */
/* Napisany przez Krzysztofa Piecucha, */
/* studenta informatyki k MISMaP UW */
/* numer albumu 332534 */
/* W projekcie zostały wykorzystane fragmenty kodu z zajęć. */
/* kodowanie UTF-8 */

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
/* Nie jest chronione semaforem! */
extern int usun_FIFO(FIFO *const fifo);

/* Jeśli wynik jest -1 należy zniszczyć kolejkę. */
/* -1 jest podawane tylko wtedy jeśli przepełni się licznik na semaforze */
/* czyli kiedy maksymalna liczba elemnetów > 32000+ */
/* Daje 1 jeśli operacja jest wykonana na pustej kolejce. */
/* Daje 0 jeśli operacja została wykonana poprawnie. */
extern int pop_FIFO(FIFO *const fifo);

/* Zawsze się powiedzie. */
/* Czeka na semaforze tak długo dopóki nie zwolni się miejsce. */
/* Daje -1 jeśli wywołanie zostaje przerwane sygnałem. */
/* Daje 1 jeśli operacja jest wykonana na pełnej kolejce. */
/* Daje 0 jeśli operacja została wykonana poprawnie. */
extern int push_FIFO(FIFO *const fifo, int16_t element);

/* Daje 0 jeśli kolejka jest pusta. */
/* Operacja zawsze się powiedzie, jeśli struktura jest w poprawnym stanie. */
extern int16_t top_FIFO(FIFO *const fifo);

/* Daje -1 jeśli operacja się nie powiodła. */
/* Wtedy jedyną możliwością jest ponowne stworzenie całej struktury. */
/* Operacja się nie powiedzie wtedy i tylko wtedy */
/* gdy maksymalna liczba elementów > 32000 z hakiem (jakiś maxint) */
/* ponieważ przepełnia się wartość na semaforze do wrzucania do kolejki. */
extern int wyczysc_FIFO(FIFO *const fifo);

/* Atomowe. Zawsze się udaje. */
/* Daje -1 jeśli wywołanie zostanie przerwane przez sygnał. */
/* Daje 1 jeśli kolejka jest pełna, 0 jeśli nie jest pełna. */
extern int jest_pelna(FIFO *const fifo);

/* Podaje liczbę zużytych bajtów. */
/* Atomowe. Zawsze się udaje. */
/* Daje SIZE_MAX jeśli wywołanie zostanie przerwane przez sygnał. */
extern size_t liczba_zuzytych_bajtow(const FIFO *const fifo);

#endif
