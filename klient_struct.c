#include <arpa/inet.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <stdlib.h>
#include <netinet/ip.h>
#include <sys/types.h>
#include "klient_struct.h"
#include "kolejka.h"
char *SITREP(klient *const);
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

static int porownaj_ipv4(struct sockaddr_in *pierwszy,
			 struct sockaddr_in *drugi)
{
	if (pierwszy->sin_port != drugi->sin_port)
		return 1;
	return pierwszy->sin_addr.s_addr - drugi->sin_addr.s_addr;
}

static int porownaj_ipv6(struct sockaddr_in6 *pierwszy,
			 struct sockaddr_in6 *drugi)
{
	if (pierwszy->sin6_port != drugi->sin6_port)
		return 1;
	return memcmp(&(pierwszy->sin6_addr),
		      &(drugi->sin6_addr), sizeof(pierwszy->sin6_addr));
}


int rowne(struct sockaddr *pierwszy, struct sockaddr *drugi)
{
	if (pierwszy == NULL && drugi == NULL)
		return 0;
	if (pierwszy == NULL || drugi == NULL)
		return 1;
	if (pierwszy->sa_family != drugi->sa_family)
		return 1;
	if (pierwszy->sa_family == AF_INET)
		return porownaj_ipv4((struct sockaddr_in *) pierwszy,
				     (struct sockaddr_in *) drugi);
	/* if (pierwszy->sa_family = AF_INET6) */
		return porownaj_ipv6((struct sockaddr_in6 *) pierwszy,
				     (struct sockaddr_in6 *) drugi);
	info("Jakiś lewy adres dostaliśmy.");
	return -1;
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
	k->min_rozmiar_ostatnio = SIZE_MAX;
	k->max_rozmiar_ostatnio = 0;
	k->spodziewany_nr_paczki = 0;
	k->kolejka = init_FIFO();
	k->potwierdzil_numer = false;
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
	memset(&k->adres_udp, 0, sizeof(k->adres_udp));
	k->czas = clock();
	return k;
}

void usun(klient *kto)
{
	if (kto == NULL)
		return;
	usun_FIFO(kto->kolejka);
	if (kto->deskryptor_tcp != -1){
		if (close(kto->deskryptor_tcp) != 0) {
			perror("Nie można zamknąć gniazda klienta.");
		}
	}
	free(kto->adres);
	free(kto->port);
	free(kto);
	/* zwolnij(3, kto->adres, kto->port, kto); */
}

char *SITREP(klient *const o_kim)
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
	o_kim->min_rozmiar_ostatnio = SIZE_MAX;
	o_kim->max_rozmiar_ostatnio = 0;
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

static size_t zlokalizuj_po_nr_klienckim(const int32_t numer_kliencki,
				  klient **const klienci,
				  const size_t MAX_KLIENTOW)
{
	size_t i;
	for (i = 0; i < MAX_KLIENTOW; ++i) {
		if (klienci[i] != NULL &&
		    klienci[i]->numer_kliencki == numer_kliencki)
			return i;
	}
	return MAX_KLIENTOW;
}

void dodaj_adresy(const int32_t numer_kliencki, struct sockaddr *adres,
		  klient **const klienci, const size_t MAX_KLIENTOW)
{
	size_t idx_klienta;
	const size_t MAX_ROZMIAR_PORTU = 20;
	/* Unikamy rzutowania potem. */
	struct sockaddr_in *arg = (struct sockaddr_in *) adres;
	struct sockaddr_in6 *arg2 = (struct sockaddr_in6 *) adres;
	struct sockaddr_in *adr_kli;
	idx_klienta = zlokalizuj_po_nr_klienckim(numer_kliencki,
						 klienci, MAX_KLIENTOW);
	if (klienci[idx_klienta]->potwierdzil_numer) {
		usun(klienci[idx_klienta]);
		klienci[idx_klienta] = NULL;
	}
	klienci[idx_klienta]->potwierdzil_numer = true;
	klienci[idx_klienta]->adres_udp.sin6_family = adres->sa_family;
	klienci[idx_klienta]->adres_udp.sin6_family = arg2->sin6_family;
	klienci[idx_klienta]->adres_udp.sin6_flowinfo = arg2->sin6_flowinfo;
	klienci[idx_klienta]->adres_udp.sin6_port = arg2->sin6_port;
	memcpy(&(klienci[idx_klienta]->adres_udp.sin6_addr),
	       &(arg2->sin6_addr), sizeof(arg2->sin6_addr));
	klienci[idx_klienta]->adres_udp.sin6_scope_id = arg2->sin6_scope_id;
	/* Wszystko się powiodło. Fajnie. */
	/* Teraz typo wpisać port tam gdzie trzeba. */
	klienci[idx_klienta]->port = malloc(MAX_ROZMIAR_PORTU);
	if (klienci[idx_klienta]->port == NULL) {
		usun(klienci[idx_klienta]);
		debug("Na klienta %"SCNd32" zabrakło pamięci.",
		      numer_kliencki);
		klienci[idx_klienta] = NULL;
		return;
	}
	/* Tu ma albo ipv6 albo ipv4 */
	if (klienci[idx_klienta]->adres_udp.sin6_family == AF_INET6) {
		if (sprintf(klienci[idx_klienta]->port, "%hu",
			    klienci[idx_klienta]->adres_udp.sin6_port) < 1) {
			usun(klienci[idx_klienta]);
			debug("Nie udało się wypisać portu klienta %"SCNd32".",
			      numer_kliencki);
			klienci[idx_klienta] = NULL;
		}
	} else if (klienci[idx_klienta]->adres_udp.sin6_family == AF_INET) {
		if (sprintf(klienci[idx_klienta]->port, "%hu",
			    ((struct sockaddr_in *)
			     &(klienci[idx_klienta]->adres_udp))->sin_port)
		    < 0) {
			usun(klienci[idx_klienta]);
			klienci[idx_klienta] = NULL;
			debug("Nie udało się wypisać portu klienta %"SCNd32".",
			      numer_kliencki);
		}
	}
	klienci[idx_klienta]->czas = clock();
}

void dodaj_klientowi_dane(void *bufor, size_t ile_danych,
			  struct sockaddr *adres, klient **const klienci,
			  const size_t MAX_KLIENTOW)
{
	size_t idx_klienta = podaj_indeks_klienta(adres, klienci, MAX_KLIENTOW);
	int32_t nr, ack, win;
	if (idx_klienta >= MAX_KLIENTOW)
		return;
	wyskub_dane_z_naglowka(bufor, &nr, &ack, &win);
	long long int nrLL = nr;
	if (klienci[idx_klienta]->spodziewany_nr_paczki != nrLL) {
		usun(klienci[idx_klienta]);
		klienci[idx_klienta] = NULL;
	}
	if (!klienci[idx_klienta]->potwierdzil_numer ||
	    dodaj(klienci[idx_klienta]->kolejka, bufor, ile_danych) != 0) {
		info("Klient %"SCNd32" dał za dużo danych, wywalamy go.",
		     klienci[idx_klienta]->numer_kliencki);
		usun(klienci[idx_klienta]);
		klienci[idx_klienta] = NULL;
	}
	klienci[idx_klienta]->czas = clock();
	++(klienci[idx_klienta]->spodziewany_nr_paczki);
}
