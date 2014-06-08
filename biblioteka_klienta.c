/* Trzeci program zaliczeniowy z sieci komputerowych. */
/* Napisany przez Krzysztofa Piecucha, */
/* studenta informatyki k MISMaP UW */
/* numer albumu 332534 */
/* W projekcie zostały wykorzystane fragmenty kodu z zajęć. */
/* kodowanie UTF-8 */


#include <assert.h>
#include <errno.h>
#include "biblioteka_klienta.h"
#include "wspolne.h"
#include "err.h"
size_t RETRANSMIT_LIMIT;

static ssize_t zrob_paczke_danych(const size_t okno,
				  const int32_t nr_paczki,
				  char **napis)
{
	const size_t MAX_ROZMIAR_DANYCH = 1400;
	static int32_t ostatnio_wyslany_nr;
	static char* ostatnio_wyslany_pakiet;
	static ssize_t ostatni_wynik;
	static bool pytal_o_taki;
	char *poczatek_danych;
	ssize_t ile_wczytano;
	int errtmp;
	const size_t MIN_ROZMIAR = 30;
	(*napis) = malloc(MTU);
	if (nr_paczki == ostatnio_wyslany_nr + 1 || nr_paczki == 0) {
		pytal_o_taki = false;
		free(ostatnio_wyslany_pakiet);
		ostatnio_wyslany_pakiet =
			zrob_naglowek(UPLOAD, nr_paczki, -1, -1, MIN_ROZMIAR +
				      min(okno, MAX_ROZMIAR_DANYCH));
		if (ostatnio_wyslany_pakiet == NULL)
			syserr("Brakuje pamięci.");
		poczatek_danych = strchr(ostatnio_wyslany_pakiet, '\n');
		++poczatek_danych;
		/* Nie chcemy robić sobie złego errno z powodu tego kawałka. */
		errtmp = errno;
		ile_wczytano = read(STDIN_FILENO, poczatek_danych,
				    min(okno, MAX_ROZMIAR_DANYCH));
		errno = errtmp;
		if (ile_wczytano <= 0) {
			free(ostatnio_wyslany_pakiet);
			free(*napis);
			*napis = NULL;
			ostatnio_wyslany_pakiet = NULL;
			return 0;
		}
		ostatni_wynik = ile_wczytano +
			(poczatek_danych - ostatnio_wyslany_pakiet);
		memcpy(*napis, ostatnio_wyslany_pakiet, (size_t) ostatni_wynik);
		ostatnio_wyslany_nr = nr_paczki;
		return ostatni_wynik;
	} else if (ostatnio_wyslany_nr == nr_paczki) {
		debug("%"SCNd32" nie doszedł ostatnio", ostatnio_wyslany_nr);
		if (!pytal_o_taki) {
			pytal_o_taki = true;
			free(*napis);
			return 0;
		} else {
			pytal_o_taki = false;
			memcpy(*napis, ostatnio_wyslany_pakiet,
			       (size_t) ostatni_wynik);
			return ostatni_wynik;
		}
	}
	return -1;
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
	ile_binarnych = rozmiar - (size_t) (tmp - dane_z_naglowkiem);
	assert(ile_binarnych > 0);
	if (write(STDOUT_FILENO, tmp, ile_binarnych) != (ssize_t) ile_binarnych)
		syserr("Write się nie udał.");
}

int daj_dane_serwerowi(const int deskryptor,
		       const int numer_paczki, const size_t okno)
{
	const char *const POCZATEK_KOMUNIKATU = "UPLOAD ";
	size_t DLUGOSC_POCZATKU = strlen(POCZATEK_KOMUNIKATU) + 10;
	char *const tekst = malloc(okno + DLUGOSC_POCZATKU);
	ssize_t ile_przeczytane;
	if (tekst == NULL)
		return 0;
	if (sprintf(tekst, "%s%i\n", POCZATEK_KOMUNIKATU, numer_paczki) < 0) {
		free(tekst);
		return 0;
	}
	ile_przeczytane = read(deskryptor, tekst + strlen(tekst), okno);
	if (ile_przeczytane < 0) {
		free(tekst);
		return 0;
	}
	if (ile_przeczytane == 0) {
		free(tekst);
		return 0;
	}
	if (wyslij_tekst(deskryptor, tekst)) {
		free(tekst);
		return -5;
	}
	free(tekst);
	return -5;
}

int obsluz_data(const evutil_socket_t gniazdo,
		const char *const wiadomosc, size_t ile_danych,
		ssize_t *const ostatnio_odebrany_ack,
		ssize_t *const ostatnio_odebrany_nr)
{
	int32_t nr, ack;
	ssize_t paczka_danych_wynik, wynik;
	size_t win;
	char *napis = NULL;
	char *dane;
	if (sscanf(wiadomosc, "DATA %"SCNd32" %"SCNd32" %zu\n",
		   &nr, &ack, &win) != 3) {
		return 0;
	}
	if ((*ostatnio_odebrany_nr) + 1 >= nr - RETRANSMIT
	    && *ostatnio_odebrany_nr != -1) {
		info("ostatnio odebrany numer to %i, a dostaliśmy %i, retransmit to %i",
		     *ostatnio_odebrany_nr, nr, nr - RETRANSMIT);
		dane = zrob_naglowek(RETRANSMIT,
				     (*ostatnio_odebrany_nr) + 1, -1, -1, 0);
		assert(strlen(dane) > 0);
		write(gniazdo, dane, strlen(dane));
		free(dane);
	} else {
		wypisz(wiadomosc, ile_danych);
	}
	paczka_danych_wynik = zrob_paczke_danych(win, ack, &napis);
	if (paczka_danych_wynik > 0) {
		wynik = write(gniazdo, napis, (size_t) paczka_danych_wynik);
		(*ostatnio_odebrany_ack) = ack;
		free(napis);
		napis = NULL;
		if (wynik < 0)
			return -1;
	} else {
		return 0;
	}
	return 0;
}

int obsluz_ack(const evutil_socket_t gniazdo, const char *const naglowek,
	       ssize_t *const ostatnio_odebrany_ack)
{
	int32_t ack;
	size_t win;
	char *wiadomosc;
	ssize_t rozmiar_wiadomosci;
	if (sscanf(naglowek, "ACK %"SCNd32" %zu", &ack, &win) != 2)
		return 0;
	rozmiar_wiadomosci = zrob_paczke_danych(win, ack, &wiadomosc);
	*ostatnio_odebrany_ack = (ssize_t) ack;
	if (rozmiar_wiadomosci <= 0)
		return 0;
	if (write(gniazdo, wiadomosc, (size_t) rozmiar_wiadomosci)
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
		if (jest_liczba_w_przedziale(MIN, MAX, tmp)) {
			if (sscanf(tmp, "%zu", &RETRANSMIT_LIMIT) != 1) {
				syserr("źle");
			}
		} else
			syserr("Źle podany parametr %s. "
			       "Oczekiwano wartości między %s a %s.\n",
			       OZNACZENIE, MIN, MAX);
	}
}
