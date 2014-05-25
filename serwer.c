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

#ifndef NDEBUG
const bool DEBUG = true;
#else
const bool DEBUG = false;
#endif

void zle_uzywane(const char *const nazwa_programu)
{
	fatal("Program uruchamia się %s\n", nazwa_programu);
}

int main(int argc, char **argv)
{
	const char *const MAXINT32 ="2147483647";
	if (ustaw_wodnego_Marka(argc, argv) != 0)
		fatal("Zle wartosci watermarkow.");


	return EXIT_SUCCESS;
}
