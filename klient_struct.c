#include <arpa/inet.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <stdlib.h>
#include "klient_struct.h"
#include "kolejka.h"
const char *SITREP(const klient *const);
void usun(klient*);
int dodaj_klienta(const evutil_socket_t, const in32_t,
		  klient **const, const size_t);
void usun_klienta(const evutil_socket_t, klient **const, const size_t);
/* Podaje indeks klienta badz dlugosc_tablicy jak klienta nie ma. */
static size_t daj_klienta(const evutil_socket_t, klient **const, const size_t);
/* wola free na argumentach. */
static void zwolnij(unsigned int, ...);
/* Ustawia klientowi adresy na podstawie deskryptora TCP. */
static int poustawiaj_adresy(klient *);
/* Robi klienta na podstaiwe deskrptora i daje wskaznik do niego lub NULL. */
static klient *zrob_klienta(evutil_socket_t);
/* Podaje pierwszego NULLa w tablicy. */
static size_t podaj_pierwszego_nulla(klient **const, const size_t);
static void zwolnij(unsigned int ile_do_zwolnienia, ...)
{
	va_list do_zwolnienia;
	unsigned int i;
	va_start(do_zwolnienia, ile_do_zwolnienia);
	for (i = 0; i < ile_do_zwolnienia; ++i)
		free(va_arg(do_zwolnienia, void *));
	va_end(do_zwolnienia);
}

static int poustawiaj_adresy(klient *kto)
{
	socklen_t dlugosc_adresu;
	if ((getpeername(kto->deskryptor_tcp,
			 (struct sockaddr *) &kto->adres_udp,
			 &dlugosc_adresu) != 0) ||
	    dlugosc_adresu != sizeof(kto->adres_udp)) {
		return -1;
	}
	if (NULL == inet_ntop(kto->adres_udp.sin6_family, &(kto->adres_udp),
			      kto->adres, INET6_ADDRSTRLEN + 1))
		return -1;
	return 0;
}

static klient *zrob_klienta(const evutil_socket_t deskryptor)
{
	klient *k = malloc(sizeof(klient));
	size_t tmp;
	const size_t MAX_ROZMIAR_PORTU = 10;
	if (k == NULL)
		return NULL;
	k->adres = NULL;
	k->port = NULL;
	k->min_rozmiar_ostatnio = 0;
	k->kolejka = init_FIFO();
	if (k->kolejka == NULL) {
		usun(k);
		return NULL;
	}
	k->adres = malloc(INET6_ADDRSTRLEN + 1);
	if (k->adres == NULL) {
		usun(k);
		return NULL;
	}
	k->deskryptor_tcp = deskryptor;
	k->sin_port = 0;
	if (poustawiaj_adresy(k) != 0) {
		usun(k);
		return NULL;
	}
	return k;
}

void usun(klient *const kto)
{
	if (kto == NULL)
		return;
	usun_FIFO(kto->kolejka);
	if (kto->deskryptor_tcp != -1){
		if (close(kto->deskryptor_tcp) != 0) {
			perror("Nie można zamknąć gniazda klienta.");
		}
	}
	zwolnij(3, kto->adres, kto->port, kto);
}

const char *SITREP(const klient *const o_kim)
{
	if (o_kim == NULL) {
		debug("Ktos chce sitrep o NULLu.");
		return NULL;
	}
	const size_t MAX_ROZMIAR_WYNIKU = 100;
	char *wynik = malloc(MAX_ROZMIAR_WYNIKU);
	if (wynik == NULL)
		return NULL;
	if (sprintf(wynik, "%s:%s FIFO: %zu/%zu (min. %zu, max. %zu)\n",
		    o_kim->adres, o_kim->port,
		    o_kim->kolejka->liczba_zuzytych_bajtow,
		    daj_FIFO_SIZE(), o_kim->min_rozmiar_ostatnio,
		    o_kim->max_rozmiar_ostatnio) <= 0) {
		free(wynik);
		wynik = NULL;
	}
	return wynik;
}

/* Podaje pierwszego NULLa */
static size_t podaj_pierwszego_nulla(klient **const klienci,
				     const size_t dlugosc_tablicy)
{
	size_t wynik;
	for (wynik = 0; wynik < dlugosc_tablicy; ++wynik)
		if (klienci[wynik] == NULL)
			return wynik;
	return dlugosc_tablicy;
}

int dodaj_klienta(const evutil_socket_t deskryptor,
		  const int32_t numer_kliencki,
		  klient **const klienci,
		  const size_t dlugosc_tablicy)
{
	size_t miejsce = podaj_pierwszego_nulla(klienci, dlugosc_tablicy);
	if (miejsce >= dlugosc_tablicy) {
		debug("Ktoś chce wrzucić klienta do pełnej tablicy.");
		return -1;
	}
	klienci[miejsce] = zrob_klienta(deskryptor);
	if (klienci[miejsce] == NULL)
		return -1;
	klienci[miejsce]->numer_kliencki = numer_kliencki;
	if (evutil_make_socket_nonblocking(klienci[miejsce]->deskryptor_tcp)
	    != 0) {
		usun(klienci[miejsce]);
		klienci[miejsce] = NULL;
		return -1;
	}
	return 0;
}

static size_t daj_klienta(const evutil_socket_t deskryptor_tcp,
			  klient **const klienci,
			  const size_t dlugosc_tablicy)
{
	size_t i;
	for (i = 0; i < dlugosc_tablicy; ++i)
		if ((klienci[i] != NULL) &&
		    klienci[i]->deskryptor_tcp == deskryptor_tcp)
			return i;
	return dlugosc_tablicy;
}

void usun_klienta(const evutil_socket_t deskryptor_tcp,
			 klient **const klienci,
			 const size_t dlugosc_tablicy)
{
	size_t klient = daj_klienta(deskryptor_tcp, klienci, dlugosc_tablicy);
	if (klient >= dlugosc_tablicy)
		return;
	usun(klienci[klient]);
	klienci[klient] = NULL;
}
