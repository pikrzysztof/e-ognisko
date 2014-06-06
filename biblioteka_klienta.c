#include <errno.h>
#include "biblioteka_klienta.h"
#include "wspolne.h"
size_t RETRANSMIT_LIMIT;

 /* Daje EOF jak jest EOF, liczba bajtow do wyslania, jak sie udalo, */
 /* BLAD_CZYTANIA jak jest blad. */
static ssize_t zrob_paczke_danych(const size_t okno,
				  const int32_t nr_paczki,
				  char **napis)
{
	static bool koniec_wejscia;
	static int32_t ostatnio_wyslany_nr;
	static char* ostatnio_wyslany_pakiet;
	static ssize_t ostatni_wynik;
	static bool pytal_o_taki;
	size_t rozmiar;
	char *poczatek_danych;
	ssize_t ile_wczytano;
	int errtmp;
	const size_t MIN_ROZMIAR = 30;
	(*napis) = malloc(MTU);
	if (*napis == NULL)
		syserr("Mało pamięci.");
	if (nr_paczki > 0 && nr_paczki == ostatnio_wyslany_nr && pytal_o_taki) {
		rozmiar = strlen(ostatnio_wyslany_pakiet) + 1;
		memcpy(*napis, ostatnio_wyslany_pakiet, rozmiar);
		debug("znowu ta sama paczka.");
		return ostatni_wynik;
	} else {
		pytal_o_taki = true;
		free(*napis);
		return 0;
	}
	pytal_o_taki = false;
	free(ostatnio_wyslany_pakiet);
	ostatnio_wyslany_pakiet = zrob_naglowek(UPLOAD, nr_paczki, -1, -1,
						MIN_ROZMIAR + okno);
	if (ostatnio_wyslany_pakiet  == NULL) {
		syserr("Pamięci brakuje.");
	}
	poczatek_danych = strchr(ostatnio_wyslany_pakiet, '\n');
	++poczatek_danych;
	ostatni_wynik = strlen(ostatnio_wyslany_pakiet);
	errtmp = errno;
	ile_wczytano = read(STDIN_FILENO, poczatek_danych, okno);
	errno = errtmp;
	ostatni_wynik += ile_wczytano;
	if (ile_wczytano <= 0) {
		free(ostatnio_wyslany_pakiet);
		ostatnio_wyslany_pakiet = NULL;
		return 0;
	}
	if (*napis == NULL) {
		syserr("Pamięci brakuje.");
	}
	memcpy(*napis, ostatnio_wyslany_pakiet, ostatni_wynik);
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

static void wypisz(const char *const dane_z_naglowkiem, size_t rozmiar)
{
	const char *tmp = strchr(dane_z_naglowkiem, '\n');
	size_t ile_binarnych;
	if (tmp == NULL)
		return;
	++tmp;
	ile_binarnych = rozmiar - (tmp - dane_z_naglowkiem);
	if (write(STDOUT_FILENO, tmp, ile_binarnych) != ile_binarnych)
		syserr("Write się nie udał.");
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
		const char *const wiadomosc, size_t ile_danych,
		ssize_t *const ostatnio_odebrany_ack,
		ssize_t *const ostatnio_odebrany_nr)
{
	int32_t nr, ack, win;
	int paczka_danych_wynik;
	ssize_t wynik;
	rodzaj_naglowka r;
	char *napis = NULL;
	char *dane;
	if (sscanf(wiadomosc, "DATA %"SCNd32" %"SCNd32" %"SCNd32"\n",
		   &nr, &ack, &win) != 3) {
		return 0;
	}
	if ((*ostatnio_odebrany_nr) + 1 >= nr - RETRANSMIT) {
		dane = zrob_naglowek(RETRANSMIT,
				     (*ostatnio_odebrany_nr) + 1, -1, -1, 0);
		write(gniazdo, dane, strlen(dane));
		free(dane);
	} else {
		dane = strchr(wiadomosc, '\n');
		++dane;
		ile_danych -= dane - wiadomosc;
		(*ostatnio_odebrany_nr) = nr;
		if (ile_danych <= 0) {
			free(napis);
			return -1;
		}
		if (write(STDOUT_FILENO, dane, ile_danych) != ile_danych)
			syserr("Nie da się pisać na stdout.");
	}
	paczka_danych_wynik = zrob_paczke_danych(win, ack, &napis);
	if (paczka_danych_wynik > 0) {
		wynik = write(gniazdo, napis, paczka_danych_wynik);
		(*ostatnio_odebrany_ack) = ack;
		free(napis);
		napis = NULL;
		if (wynik <= 0)
			return -1;
	} else {
		return 0;
	}
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

void ustaw_retransmit_limit(int argc, char *const *const argv)
{
	const char *const OZNACZENIE = "-X";
	const size_t DOMYSLNIE = 10;
	const char *const MIN = "0";
	const char *const MAX = "40";
	const char *const tmp = daj_opcje(OZNACZENIE, argc, argv);
	if (tmp == NULL) {
		RETRANSMIT_LIMIT = DOMYSLNIE;
	} else {
		if (jest_liczba_w_przedziale(MIN, MAX, tmp))
			RETRANSMIT_LIMIT = atoi(tmp);
		else
			syserr("Źle podany parametr %s. "
			       "Oczekiwano wartości między %s a %s.\n",
			       OZNACZENIE, MIN, MAX);
	}
}
