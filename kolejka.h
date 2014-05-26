/* Trzeci program zaliczeniowy z sieci komputerowych. */
/* Napisany przez Krzysztofa Piecucha, */
/* studenta informatyki k MISMaP UW */
/* numer albumu 332534 */
/* W projekcie zostały wykorzystane fragmenty kodu z zajęć. */
/* kodowanie UTF-8 */

#ifndef _KOLEJKA_H_
#define _KOLEJKA_H_

#include <inttypes.h>
#include <stdlib.h>

enum stan_FIFO {
	ACTIVE,
	FILLING
};

typedef struct {
	enum stan_FIFO stan;
	size_t liczba_zuzytych_bajtow;  /* Stopien zapelniena kolejki. */
	void *kolejka;
} FIFO;

/* Ustawia watermarki na pozadane wartosci,  */
/* rzuca -1 jak cos sie nie zgadza, 0 jak sie udaje. */
extern void ustaw_wodnego_Marka(FIFO *const fifo);

extern void init_wodnego_Marka(const int argc, char *const *const argv);

/* Ustawia rozmiar fifo. */
/* Daje -1 jak sie cos nie zgadza, 0 jak sie uda. */
extern void init_rozmiar_fifo(const int argc, char *const *const argv);

extern FIFO* init_FIFO();

extern void usun_FIFO(FIFO *const fifo);

extern size_t daj_FIFO_SIZE();

#endif
