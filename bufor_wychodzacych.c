#include "wspolne.h"
#include "biblioteka_serwera.h"

static size_t BUF_LEN;

void ustaw_rozmiar_wychodzacych(const int argc, char *const *const argv)
{
	size_t domyslnie = 10;
	const char *const OZNACZENIE = "-X";
	const char *const MAX_WARTOSC = "2147483647";
	const char *const MIN_WARTOSC = "0";
	const char *tmp;
	if (jest_oznaczenie(argc, argv, OZNACZENIE)) {
		tmp = daj_opcje(OZNACZENIE, argc, argv);
		if (tmp == NULL)
			fatal("Parametr %s jest błednie podany.", OZNACZENIE);
		if (!jest_liczba_w_przedziale(MIN_WARTOSC, MAX_WARTOSC, tmp))
			fatal("Parametr %s jest błędnie podany.", OZNACZENIE);
		BUF_LEN = atoi(tmp);
	}
}
