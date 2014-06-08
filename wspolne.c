/* Trzeci program zaliczeniowy z sieci komputerowych. */
/* Napisany przez Krzysztofa Piecucha, */
/* studenta informatyki k MISMaP UW */
/* numer albumu 332534 */
/* W projekcie zostały wykorzystane fragmenty kodu z zajęć. */
/* kodowanie UTF-8 */


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

const char *const OZNACZENIE_PARAMETRU_PORTU = "-p";
const char *const DOMYSLNY_NUMER_PORTU = "12534";
const size_t MTU = 2000;
bool jest_int32(const char *const liczba)
{
	const char *const MAX_INT32 = "2147483647";
	if (liczba == NULL)
		return false;
	return jest_liczba_w_przedziale("0", MAX_INT32, liczba);
}

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

void info(const char *fmt, ...)
{
	va_list  fmt_args;
	fprintf(stderr, "INFO: ");
	va_start(fmt_args, fmt);
	vfprintf(stderr, fmt, fmt_args);
	va_end(fmt_args);
	fprintf(stderr, "\n");
}

bool wyslij_numer_kliencki(int deskryptor, int32_t numer_kliencki)
{
	const size_t ROZMIAR_BUFORA = 30;
	ssize_t dlugosc_danych;
	char *bufor = malloc(ROZMIAR_BUFORA * sizeof(char));
	sprintf(bufor, "CLIENT %"SCNd32"\n", numer_kliencki);
	debug(" KOMUNIKAT %s", bufor);
	dlugosc_danych = (ssize_t) strlen(bufor);
	assert(bufor[dlugosc_danych] == '\0');
	debug("wysylam wiadomosc z nr klienckim %s", bufor);
	assert(dlugosc_danych > 0);
	if (write(deskryptor, bufor, (size_t) dlugosc_danych * sizeof(char)) !=
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
	const size_t MAX_ROZMIAR_WIADOMOSCI_POWITALNEJ = 30;
	char *przyjmowane = malloc(MAX_ROZMIAR_WIADOMOSCI_POWITALNEJ);
	const char *const MAX_NUMER_KLIENTA = "2147483647";
	const char *const MIN_NUMER_KLIENTA = "0";
	const ssize_t MIN_DLUGOSC_WIADOMOSCI = 9;
	const ssize_t POCZATEK_NUMERU = 7;
	int32_t wynik;
	ssize_t ile_przeczytane = read(deskryptor, przyjmowane,
				       MAX_ROZMIAR_WIADOMOSCI_POWITALNEJ);
	if (ile_przeczytane < MIN_DLUGOSC_WIADOMOSCI ||
	    (size_t) ile_przeczytane == MAX_ROZMIAR_WIADOMOSCI_POWITALNEJ) {
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
	ssize_t ile_wyslac = (ssize_t) strlen(tekst) * (ssize_t) sizeof(char);
	return (write(deskryptor, tekst, (size_t) ile_wyslac) == ile_wyslac);
}

ssize_t dopisz_na_koncu(char *const poczatek, const char *const fmt, ...)
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
	(poczatek + dlugosc_drugiego)[pierwszy] = '\0';
}

size_t max(const size_t a, const size_t b)
{
	if (a > b)
		return a;
	return b;
}

size_t min(const size_t a, const size_t b)
{
	if (a > b)
		return (size_t) b;
	return a;
}

/* sscanf działa za wolno. */
static bool rowne(size_t dlugosc, const char *const wzorzec,
		  const char *const arg)
{
	size_t i;
	for (i = 0; i < dlugosc; ++i) {
		if (wzorzec[i] != arg[i])
			return false;
	}
	return true;
}

rodzaj_naglowka rozpoznaj_naglowek(const char *const naglowek)
{
	if (rowne(6, "CLIENT", naglowek))
		return CLIENT;
	if (rowne(6, "UPLOAD", naglowek))
		return UPLOAD;
	if (rowne(3, "ACK", naglowek))
		return ACK;
	if (rowne(10, "RETRANSMIT", naglowek))
		return RETRANSMIT;
	if (rowne(9, "KEEPALIVE", naglowek))
		return KEEPALIVE;
	if (rowne(4, "DATA", naglowek))
		return DATA;
	return INNY;
}

char* podaj_slowo(const char *const naglowek, size_t ktore)
{
	const char *poczatek = naglowek;
	const char *koniec;
	char *wynik = NULL;
	ssize_t ile_kopiowac, ile_alokowac;
	while (ktore --> 0) {
		if (poczatek == NULL)
			return NULL;
		poczatek = strchr(poczatek, ' ');
		if (poczatek != NULL)
			++poczatek;
	}
	if (poczatek == NULL)
		return NULL;
	koniec = strchr(poczatek, ' ');
	if (koniec == NULL)
		koniec = strchr(poczatek, '\0');
	ile_kopiowac = koniec - poczatek;
	ile_alokowac = ile_kopiowac + 1;
	assert(ile_kopiowac >= 0 && ile_alokowac >= 0);
	wynik = malloc((size_t) ile_alokowac);
	memcpy(wynik, poczatek, (size_t) ile_kopiowac);
	(ile_alokowac - 1)[wynik] = '\0';
	return wynik;
}

int32_t wyskub_liczbe(const char *const ktora, size_t ktore_slowo)
{
	char *liczba = podaj_slowo(ktora, ktore_slowo);
	int32_t wynik = -1;
	if (jest_int32(liczba)) {
		wynik = atoi(liczba);
	}
	free(liczba);
	return wynik;
}

int wyskub_dane_z_naglowka(const char *const naglowek,
			   int32_t *const nr, int32_t *const ack,
			   int32_t *const win)
{
	rodzaj_naglowka r = rozpoznaj_naglowek(naglowek);
	*nr = -1;
	*ack = -1;
	*win = -1;
	switch (r) {
	case CLIENT:
		*nr = wyskub_liczbe(naglowek, 1);
		return 0;
	case DATA:
		*ack = wyskub_liczbe(naglowek, 2);
		*win = wyskub_liczbe(naglowek, 3);
		if (*ack == -1 || *win == -1)
			return -1;
	case UPLOAD:
	case RETRANSMIT:
		*nr = wyskub_liczbe(naglowek, 1);
		if (*nr == -1)
			return -1;
		return 0;
	case ACK:
		*ack = wyskub_liczbe(naglowek, 1);
		*win = wyskub_liczbe(naglowek, 2);
		if (*ack == -1 || *win == -1)
			return -1;
		return 0;
	default:
		debug("Nie udało się dopasować do wzorca.");
		return -1;
	}
	return 0;
}

static char* zrob_ack(const int32_t ack, const int32_t win)
{
	const size_t MAX_ROZMIAR_NAGLOWKA = 30;
	const ssize_t MINIMALNA_DLUGOSC_NAGLOWKA = 8;
	char *const wynik = malloc(MAX_ROZMIAR_NAGLOWKA);
	if (wynik == NULL)
		return NULL;
	if (sprintf(wynik, "ACK %"SCNd32" %"SCNd32"\n", ack, win)
	    < MINIMALNA_DLUGOSC_NAGLOWKA) {
		free(wynik);
		return NULL;
	}
	return wynik;
}

static char* zrob_upload(const int32_t nr, const size_t rozmiar_wyniku)
{
	char *const wynik = malloc(rozmiar_wyniku);
	const ssize_t MINIMALNA_DLUGOSC_NAGLOWKA = 9;
	if (wynik == NULL)
		return NULL;
	if (sprintf(wynik, "UPLOAD %"SCNd32"\n", nr)
	    < MINIMALNA_DLUGOSC_NAGLOWKA) {
		free(wynik);
		return NULL;
	}
	return wynik;
}

static char* zrob_data(const int32_t nr, const int32_t ack,
			     const int32_t win, const size_t rozmiar_wyniku)
{
	char *const wynik = malloc(rozmiar_wyniku);
	const ssize_t MINIMALNA_DLUGOSC_NAGLOWKA = 11;
	if (wynik == NULL)
		return NULL;
	if (sprintf(wynik, "DATA %"SCNd32" %"SCNd32" %"SCNd32"\n",
		    nr, ack, win) < MINIMALNA_DLUGOSC_NAGLOWKA) {
		free(wynik);
		return NULL;
	}
	return wynik;
}

static char* zrob_keepalive()
{
	char *const wynik = malloc(40);
	const ssize_t MINIMALNA_DLUGOSC_NAGLOWKA = 10;
	if (wynik == NULL)
		return NULL;
	if (sprintf(wynik, "KEEPALIVE\n") < MINIMALNA_DLUGOSC_NAGLOWKA) {
		free(wynik);
		return NULL;
	}
	return NULL;
}

static char* zrob_retransmit(const int32_t nr)
{
	char *const wynik = malloc(40);
	const ssize_t MINIMALNA_DLUGOSC_NAGLOWKA = 10;
	if (wynik == NULL)
		return NULL;
	if (sprintf(wynik, "RETRANSMIT %"SCNd32"\n", nr)
	    < MINIMALNA_DLUGOSC_NAGLOWKA) {
		free(wynik);
		return NULL;
	}
	return wynik;
}

static char* zrob_client(const int32_t clientid)
{
	char *const wynik = malloc(40);
	const ssize_t MINIMALNA_DLUGOSC_NAGLOWKA = 8;
	if (wynik == NULL)
		return NULL;
	if (sprintf(wynik, "CLIENT %"SCNd32"\n", clientid)
	    < MINIMALNA_DLUGOSC_NAGLOWKA) {
		free(wynik);
		return NULL;
	}
	return wynik;
}

char* zrob_naglowek(const rodzaj_naglowka r,
		    const int32_t nr, const int32_t ack,
		    const int32_t win, const size_t rozmiar_wyniku)
{
	switch (r) {
	case CLIENT:
		return zrob_client(nr);
	case RETRANSMIT:
		return zrob_retransmit(nr);
	case UPLOAD:
		return zrob_upload(nr, rozmiar_wyniku);
	case DATA:
		return zrob_data(nr, ack, win, rozmiar_wyniku);
	case ACK:
		return zrob_ack(ack, win);
	case KEEPALIVE:
		return zrob_keepalive();
	default:
		debug("Ktos chce dziwny naglowek.");
		return NULL;
	}
}
