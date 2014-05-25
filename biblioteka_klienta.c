#include "biblioteka_klienta.h"

 /* Daje EOF jak jest EOF, liczba bajtow do wyslania, jak sie udalo, */
 /* BLAD_CZYTANIA jak jest blad. */
static ssize_t zrob_paczke_danych(const size_t okno, const int32_t nr_paczki,
			      char **napis)
{
	static bool koniec_wejscia;
	static int32_t ostatnio_wyslany_nr;
	static char* ostatnio_wyslany_pakiet;
	static ssize_t ostatni_wynik;
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
		return ostatni_wynik;
	}
	free(ostatnio_wyslany_pakiet);
	if (koniec_wejscia) {
		return EOF;
	}
	ostatnio_wyslany_pakiet = zrob_naglowek(UPLOAD, nr_paczki, -1, -1,
						MIN_ROZMIAR + okno);
	tmp = malloc(okno + 5);
	if (tmp == NULL || ostatnio_wyslany_pakiet  == NULL) {
		free(ostatnio_wyslany_pakiet);
		ostatnio_wyslany_pakiet = NULL;
		return BLAD_CZYTANIA;
	}
	ostatni_wynik = strlen(ostatnio_wyslany_pakiet);
	ile_wczytano = read(STDIN_FILENO, tmp, okno);
	ostatni_wynik += ile_wczytano;
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
	return ostatni_wynik;
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

int obsluz_data(const evutil_socket_t gniazdo,
		const char *const naglowek,
		ssize_t *const ostatnio_odebrany_ack,
		ssize_t *const ostatnio_odebrany_nr)
{
	int32_t nr, ack, win;
	int paczka_danych_wynik;
	rodzaj_naglowka r;
	char *napis;
	char *dane;
	ssize_t ile_danych;
	size_t UDP_MTU = 1600;
	if (wyskub_dane_z_naglowka(naglowek, &nr, &ack, &win) == -1) {
		return 0;
	}
	paczka_danych_wynik = zrob_paczke_danych(win, ack, &napis);
	if (paczka_danych_wynik == 1) {
		write(gniazdo, napis, strlen(napis));
		(*ostatnio_odebrany_ack) = ack;
	} else if (paczka_danych_wynik == BLAD_CZYTANIA) {
		return 0;
	}
	dane = malloc(UDP_MTU);
	ile_danych = read(gniazdo, dane, UDP_MTU);
	(*ostatnio_odebrany_nr) = nr;
	if (ile_danych <= 0) {
		free(dane);
		return -1;
	}
	if (write(STDOUT_FILENO, dane, ile_danych) != ile_danych) {
		perror("Nie mozna cos pisac na stdout!");
		free(dane);
		return 0;
	}
	free(dane);
	return 0;
}

int obsluz_ack(const evutil_socket_t gniazdo, const char *const naglowek,
	       ssize_t *const ostatnio_odebrany_ack,
	       ssize_t *const ostatnio_odebrany_nr)
{
	int32_t nr, ack, win;
	char *wiadomosc;
	ssize_t rozmiar_wiadomosci;
	if (wyskub_dane_z_naglowka(naglowek, &nr, &ack, &win) != 0)
		return 0;
	rozmiar_wiadomosci = zrob_paczke_danych(win, ack, &wiadomosc);
	*ostatnio_odebrany_ack = ack;
	if (rozmiar_wiadomosci == EOF)
		return 0;
	if (rozmiar_wiadomosci == BLAD_CZYTANIA)
		return 0;
	if (write(gniazdo, wiadomosc, rozmiar_wiadomosci)
	    != rozmiar_wiadomosci) {
		free(wiadomosc);
		return -1;
	}
	free(wiadomosc);
	return 0;

}
