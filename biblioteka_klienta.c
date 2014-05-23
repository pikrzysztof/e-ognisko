#include "biblioteka_klienta.h"

int zrob_paczke_danych(const size_t okno, const int32_t nr_paczki, char **napis)
{
	static bool koniec_wejscia;
	static int32_t ostatnio_wyslany_nr;
	static char* ostatnio_wyslany_pakiet;
	size_t rozmiar;
	char *tmp;
	ssize_t ile_wczytano;
	const size_t MIN_ROZMIAR = 30;
	if (nr_paczki > 0 && nr_paczki == ostatnio_wyslany_nr) {
		rozmiar = strlen(ostatnio_wyslany_pakiet) + 1;
		*napis = malloc(rozmiar);
		if (*napis == NULL)
			return BLAD_CZYTANIA;
		memcpy(*napis, ostatnio_wyslany_pakiet, rozmiar);
		if (*napis == NULL)
			return BLAD_CZYTANIA;
		return 0;
	}
	free(ostatnio_wyslany_pakiet);
	ostatnio_wyslany_pakiet = zrob_naglowek(UPLOAD, nr_paczki, -1, -1,
						MIN_ROZMIAR + okno);
	tmp = malloc(okno + 5);
	if (tmp == NULL) {
		free(ostatnio_wyslany_pakiet);
		ostatnio_wyslany_pakiet = NULL;
		return BLAD_CZYTANIA;
	}
	ile_wczytano = read(STDIN_FILENO, tmp, okno);
	if (ile_wczytano < 0) {
		free(ostatnio_wyslany_pakiet);
		ostatnio_wyslany_pakiet = NULL;
		free(tmp);
		return BLAD_CZYTANIA;
	}
	if (ile_wczytano == 0) {
		koniec_wejscia = true;
		free(tmp);
		free(ostatnio_wyslany_pakiet);
		return EOF;
	}
	konkatenacja(ostatnio_wyslany_pakiet, tmp, ile_wczytano);
	free(tmp);
	*napis = malloc(okno + MIN_ROZMIAR);
	if (*napis == NULL) {
		free(ostatnio_wyslany_pakiet);
		ostatnio_wyslany_pakiet = NULL;
		return BLAD_CZYTANIA;
	}
	memcpy(*napis, ostatnio_wyslany_pakiet,
	       strlen(ostatnio_wyslany_pakiet) + 1);
	return 0;
}

void wyczysc(evutil_socket_t *deskryptor,
	     evutil_socket_t *deskryptor_2,
	     struct event_base *baza_zdarzen,
 	     unsigned int liczba_wydarzen, ...)
{
	va_list wydarzenia;
	unsigned int i;
	if (deskryptor != NULL) {
		if (close(*deskryptor) != 0)
			perror("Nie udało się zamknąć deskryptora.");
	}
	if (deskryptor_2 != NULL) {
		if (close(*deskryptor_2) != 0)
			perror("Nie udało się zamknąć deskryptora.");
	}
	va_start(wydarzenia, liczba_wydarzen);
	for (i = 0; i < liczba_wydarzen; ++i) {
		event_free(va_arg(wydarzenia, struct event *));
	}
	va_end(wydarzenia);
	if (baza_zdarzen != NULL) {
		event_base_free(baza_zdarzen);
	}
}

bool popros_o_retransmisje(const int deskryptor, const int numer)
{
	const size_t MAX_ROZMIAR_BUFORA = 30;
	char *const tekst = malloc(MAX_ROZMIAR_BUFORA * sizeof(char));
	bool wynik;
	if (tekst == NULL)
		return false;
	if (sprintf(tekst, "RETRANSMIT %i\n", numer) < 0) {
		free(tekst);
		return false;
	}
	wynik = wyslij_tekst(deskryptor, tekst);
	free(tekst);
	return wynik;
}

/* Daje BLAD_CZYTANIA jak cos sie nie uda, EOF jak jest koniec pliku, */
/* 0 jak sie uda. */
int daj_dane_serwerowi(const int deskryptor,
		       const int numer_paczki, const size_t okno)
{
	const char *const POCZATEK_KOMUNIKATU = "UPLOAD ";
	size_t DLUGOSC_POCZATKU = strlen(POCZATEK_KOMUNIKATU) + 10;
	char *const tekst = malloc(okno + DLUGOSC_POCZATKU);
	ssize_t ile_przeczytane;
	if (tekst == NULL)
		return BLAD_CZYTANIA;
	if (sprintf(tekst, "%s%i\n", POCZATEK_KOMUNIKATU, numer_paczki) < 0) {
		free(tekst);
		return BLAD_CZYTANIA;
	}
	ile_przeczytane = read(deskryptor, tekst + strlen(tekst), okno);
	if (ile_przeczytane < 0) {
		free(tekst);
		return BLAD_CZYTANIA;
	}
	if (ile_przeczytane == 0) {
		free(tekst);
		return EOF;
	}
	if (wyslij_tekst(deskryptor, tekst)) {
		free(tekst);
		return 0;
	}
	free(tekst);
	return BLAD_CZYTANIA;
}

int obsluz_data(evutil_socket_t gniazdo, const char *const naglowek,
		       ssize_t *ostatnio_odebrany_ack,
		       ssize_t *ostatnio_odebrany_nr)
{
	return 5;
}
