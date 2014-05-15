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

#ifndef NDEBUG
static const bool DEBUG = true;
#else
static const bool DEBUG = false;
#endif

const char *const DOMYSLNY_NUMER_PORTU = "12534";

bool jest_liczba_w_przedziale(const char *const poczatek,
				     const char *const koniec,
				     const char *const liczba)
{
	const int DLUGOSC_POCZATKU = strlen(poczatek);
	const int DLUGOSC_KONCA = strlen(koniec);
	const int DLUGOSC = strlen(liczba);
        int i;
        if (DLUGOSC > DLUGOSC_KONCA || DLUGOSC < DLUGOSC_POCZATKU)
                return false;
        if (liczba[0] == '0')
                return (DLUGOSC == 1 && strcmp(poczatek, "0") == 0);
        for (i = 0; i < DLUGOSC; i++)
                if (liczba[i] < '0' || liczba[i] > '9')
                        return false;
        return ((strcmp(liczba, koniec) <= 0) &&
		(strcmp(liczba, poczatek) >= 0));
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
	assert(bufor[dlugosc_danych] != '\0');
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
	ssize_t tmp;
	int i = -1;
	do {
		++i;
		tmp = read(deskryptor, przyjmowane + i, 1);
	} while (przyjmowane[i] != '\n' || tmp == 0);
	if (tmp == 0)
		return -1;
	if (strcmp(przyjmowane, "CLIENT") != 0)
		return -1;
	przyjmowane[i] = '\0';
	if (!jest_liczba_w_przedziale(MIN_NUMER_KLIENTA, MAX_NUMER_KLIENTA,
				      przyjmowane + i))
	    return -1;
	return atoi(przyjmowane + i);
}

extern bool wyslij_tekst(int deskryptor, const char *const tekst)
{
	ssize_t ile_wyslac = strlen(tekst) * sizeof(char);
	return (write(deskryptor, tekst, ile_wyslac) == ile_wyslac);
}
