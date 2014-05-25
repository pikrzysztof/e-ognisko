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
	size_t liczba_zuzytych_miejsc;  /* Stopien zapelniena kolejki. */
	size_t maksymalna_liczba_elementow;
	size_t pierwszy;        /* Wskazuje na pierwszy element kolejki. */
	int16_t *kolejka;
} FIFO;

size_t FIFO_SIZE /* = 10560 */;
size_t FIFO_LOW_WATERMARK /* = 0 */;
size_t FIFO_HIGH_WATERMARK /* = FIFO_SIZE */;

/* Ustawia watermarki na pozadane wartosci,  */
/* rzuca -1 jak cos sie nie zgadza, 0 jak sie udaje. */
extern int ustaw_wodnego_Marka(const int argc, const char *const *const argv);

/* Ustawia rozmiar fifo. */
/* Daje -1 jak sie cos nie zgadza, 0 jak sie uda. */
extern int ustaw_rozmiar_fifo(const int argc, const char *const *const argv);

#endif
