/* Trzeci program zaliczeniowy z sieci komputerowych. */
/* Napisany przez Krzysztofa Piecucha, */
/* studenta informatyki k MISMaP UW */
/* numer albumu 332534 */
/* W projekcie zostały wykorzystane fragmenty kodu z zajęć. */
/* kodowanie UTF-8 */

#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include "err.h"
#include "biblioteka_serwera.h"
#include "kolejka.h"
#include "bufor_wychodzacych.h"
#ifndef NDEBUG
const bool DEBUG = true;
#else
const bool DEBUG = false;
#endif

int main(int argc, char **argv)
{
	const char *const MAXINT32 ="2147483647";
	ustaw_rozmiar_fifo(argc, argv);
	ustaw_wodnego_Marka(argc, argv);
	ustaw_rozmiar_wychodzacych(argc, argv);


	return EXIT_SUCCESS;
}
