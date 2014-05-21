#include "wspolne.h"
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <stdarg.h>
#include <stdio.h>
#include <errno.h>
#include <event2/event.h>
#include <assert.h>
#include <fcntl.h>

#ifndef NDEBUG
static const bool DEBUG = true;
#else
static const bool DEBUG = false;
#endif

/* Takim oto sposobem będzie zawsze różne od EOF i 0. */
const ssize_t BLAD_CZYTANIA = EOF + 1 >= 0 ? -5 : EOF + 1;
const char *const DOMYSLNY_NUMER_PORTU = "12534";

bool ustaw_gniazdo_nieblokujace(const int gniazdo)
{
	int biezace_flagi = fcntl(gniazdo, F_GETFL, 0);
	if (biezace_flagi == -1)
		return false;
	return (fcntl(gniazdo, F_SETFL, biezace_flagi | O_NONBLOCK) != -1);
}

static bool same_cyfry(const char *const liczba)
{
	int i;
	const int DLUGOSC = strlen(liczba);
	for (i = 0; i < DLUGOSC; ++i)
		if (liczba[i] < '0' || liczba[i] > '9')
			return false;
	return true;
}

bool jest_liczba_w_przedziale(const char *const poczatek,
			      const char *const koniec,
			      const char *const liczba)
{
	const int DLUGOSC_POCZATKU = strlen(poczatek);
	const int DLUGOSC_KONCA = strlen(koniec);
	const int DLUGOSC = strlen(liczba);
        if (DLUGOSC > DLUGOSC_KONCA || DLUGOSC < DLUGOSC_POCZATKU)
                return false;
        if (liczba[0] == '0')
                return (DLUGOSC == 1 && strcmp(poczatek, "0") == 0);
	if (!same_cyfry(liczba))
		return false;
	/* Z samych cyfr i ma posrednia dlugosc. */
	if (DLUGOSC > DLUGOSC_POCZATKU && DLUGOSC < DLUGOSC_KONCA)
		return true;
	if (DLUGOSC == DLUGOSC_KONCA)
		return (strcmp(liczba, koniec) <= 0);
	if (DLUGOSC == DLUGOSC_POCZATKU)
		return (strcmp(liczba, poczatek) >= 0);
	/* Nigdy nie dojdzie do ponizszej linijki. */
	return false;
}



bool wlasciwy_port(const char *const numer_portu)
{
	const char *const MIN_NUMER_PORTU = "0";
        const char *const MAX_NUMER_PORTU = "65535";
	return jest_liczba_w_przedziale(MIN_NUMER_PORTU, MAX_NUMER_PORTU,
					numer_portu);
}

const char* daj_opcje(const char *const nazwa_opcji,
		      const int rozmiar_tablicy_opcji,
		      char *const *const tablica_opcji)
{
	int i;
	for (i = 0; i < rozmiar_tablicy_opcji - 1; ++i) {
		if (strcmp(nazwa_opcji, tablica_opcji[i]) == 0)
			return tablica_opcji[i + 1];
	}
	return NULL;
}

/* Adres w wersjii IPv6 powinien mieć przynajmniej jeden dwukropek. */
bool jest_ipv6(const char *const domniemany_adres)
{
	int i;
	for (i = 0; domniemany_adres[i] != '\0'; ++i) {
		if (domniemany_adres[i] == ':')
			return true;
	}
	return false;
}

struct addrinfo* podaj_adres_binarny(const char* const ludzki_adres,
				     const char* const port,
				     const int protokol, const int typ_gniazda)
{
        struct addrinfo addr_hints;
        struct addrinfo *addr_result;
        (void) memset(&addr_hints, 0, sizeof(struct addrinfo));
	if (jest_ipv6(ludzki_adres))
		addr_hints.ai_family = AF_INET6;
	else
		addr_hints.ai_family = AF_INET;
        addr_hints.ai_socktype = typ_gniazda;
        addr_hints.ai_protocol = protokol;
        addr_hints.ai_flags = 0;
        addr_hints.ai_addrlen = 0;
        addr_hints.ai_addr = NULL;
        addr_hints.ai_canonname = NULL;
        addr_hints.ai_next = NULL;
        if (getaddrinfo(ludzki_adres, port, &addr_hints, &addr_result) != 0)
		return NULL;
        return addr_result;
}

int zrob_gniazdo(const int typ, const char *const ludzki_adres)
{
	if (jest_ipv6(ludzki_adres))
		return socket(AF_INET6, typ, 0);
	return socket(AF_INET, typ, 0);
}

void debug(const char *fmt, ...)
{
	va_list fmt_args;
	if (!DEBUG)
		return;
	fprintf(stderr, "DEBUG: ");
	va_start(fmt_args, fmt);
	vfprintf(stderr, fmt, fmt_args);
	va_end(fmt_args);
	fprintf(stderr, "\n");
	if (errno != 0) {
		fprintf(stderr, "ERROR: (%d; %s)\n", errno, strerror(errno));
	}
}

bool wyslij_numer_kliencki(int deskryptor, int32_t numer_kliencki)
{
	const size_t ROZMIAR_BUFORA = 30;
	ssize_t dlugosc_danych;
	char *bufor = malloc(ROZMIAR_BUFORA * sizeof(char));
	sprintf(bufor, "CLIENT %"SCNd32"\n", numer_kliencki);
	dlugosc_danych = strlen(bufor);
	assert(bufor[dlugosc_danych] == '\0');
	if (write(deskryptor, bufor, dlugosc_danych * sizeof(char)) !=
	    dlugosc_danych) {
		perror("Nie udało się wysłać numeru klienckiego.\n");
		free(bufor);
		return false;
	}
	free(bufor);
	return true;
}

int32_t odbierz_numer_kliencki(int deskryptor)
{
	size_t max_rozmiar_wiadomosci_powitalnej = 30;
	char *przyjmowane = malloc(sizeof(char) * max_rozmiar_wiadomosci_powitalnej);
	const char *const MAX_NUMER_KLIENTA = "2147483647";
	const char *const MIN_NUMER_KLIENTA = "0";
	const size_t MIN_DLUGOSC_WIADOMOSCI = 9;
	const size_t POCZATEK_NUMERU = 7;
	int32_t wynik;
	ssize_t ile_przeczytane =
		czytaj_do_konca_linii(deskryptor, przyjmowane,
				      max_rozmiar_wiadomosci_powitalnej);
	if (ile_przeczytane < MIN_DLUGOSC_WIADOMOSCI) {
		free(przyjmowane);
		perror("Jakiś dziwny numer kliencki.");
		return -1;
	}
	/* Usuwamy nowa linie, zeby moc sprawdzicz, czy to numer kliencki. */
	przyjmowane[ile_przeczytane - 1] = '\0';
	if (!jest_liczba_w_przedziale(MIN_NUMER_KLIENTA, MAX_NUMER_KLIENTA,
				      przyjmowane + POCZATEK_NUMERU)) {
		free(przyjmowane);
		perror("Jakiś dziwny numer kliencki!");
		return -1;
	}
	wynik = atoi(przyjmowane + POCZATEK_NUMERU);
	free(przyjmowane);
	return wynik;
}

bool wyslij_tekst(int deskryptor, const char *const tekst)
{
	ssize_t ile_wyslac = strlen(tekst) * sizeof(char);
	return (write(deskryptor, tekst, ile_wyslac) == ile_wyslac);
}

ssize_t dopisz_na_koncu(char *const poczatek,
			const char *const fmt, ...)
{
	size_t dlugosc = strlen(poczatek);
	va_list fmt_args;
	ssize_t wynik;
	va_start(fmt_args, fmt);
	wynik = vsprintf(poczatek + dlugosc, fmt, fmt_args);
	va_end(fmt_args);
	return wynik;
}


void konkatenacja(char *const pierwszy, const char *const drugi,
		  size_t dlugosc_drugiego)
{
	size_t poczatek = strlen(pierwszy);
	memcpy(pierwszy + poczatek, drugi, dlugosc_drugiego);
}

ssize_t czytaj_do_konca_linii(const int deskryptor,
			      char *const bufor,
			      const ssize_t rozmiar_bufora)
{
	ssize_t ile_wczytano;
	ssize_t i = -1;
	do {
		++i;
		ile_wczytano = read(deskryptor, bufor + i, 1);

	} while ((ile_wczytano == 1) && (i < rozmiar_bufora - 2)
		 && (bufor[i] != '\n'));
	if (ile_wczytano == -1)
		return BLAD_CZYTANIA;
	bufor[i + 1] = '\0';
	if (ile_wczytano == 0)
		return EOF;
	return i;
}

ssize_t czytaj_do_vectora(const int deskryptor, char **wynik)
{
	ssize_t ile_wczytano, i;
	const size_t MNOZNIK = 2;
	size_t dlugosc_bufora = 4;
	*wynik = malloc(dlugosc_bufora);
	do {
		++i;
		if (dlugosc_bufora <= i + 2)
			*wynik = realloc(wynik, dlugosc_bufora *= MNOZNIK);
		ile_wczytano = read(deskryptor, (*wynik) + i, 1);
	} while ((ile_wczytano == 1) && ((*wynik)[i] != '\n'));
	if (ile_wczytano == -1) {
		free(*wynik);
		*wynik = NULL;
		return BLAD_CZYTANIA;
	}
	if (ile_wczytano == 0)
		return EOF;
	(*wynik)[i + 1] = '\0';
	return i;
}

int max(const int a, const int b)
{
	if (a > b)
		return a;
	return b;
}

int min(const int a, const int b)
{
	if (a > b)
		return b;
	return a;
}
