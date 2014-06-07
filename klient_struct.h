#ifndef _KLIENT_STRUCT_H
#define _KLIENT_STRUCT_H
#include <time.h>
#include "kolejka.h"
#include <event2/event.h>
#include <stdbool.h>

struct jeszcze_dziwniejsze_imie {
	FIFO *kolejka;
	char *adres;
	char *port;
	size_t min_rozmiar_ostatnio;
	size_t max_rozmiar_ostatnio;
	size_t spodziewany_nr_paczki;
	evutil_socket_t deskryptor_tcp;
	in_port_t sin_port;
	struct sockaddr_in6 adres_udp;
	int32_t numer_kliencki;
	clock_t czas;
	bool potwierdzil_numer;
};

extern char *SITREP(klient *const o_kim);

extern void usun(klient *kto);

extern int dodaj_klienta(const evutil_socket_t deskryptor,
			 const int32_t numer_kliencki,
			 klient **const klienci,
			 const size_t dlugosc_tablicy);

extern void usun_klienta(const evutil_socket_t deskryptor_tcp,
			 klient **const klienci,
			 const size_t dlugosc_tablicy);

extern int rowne(struct sockaddr_in6 *pierwszy, struct sockaddr_in6 *drugi);

extern void dodaj_adresy(const int32_t numer_kliencki,
			 struct sockaddr_in6 *adres,
			klient **const klienci, const size_t MAX_KLIENTOW);

/* extern void wywal(struct sockaddr* adres, klient **klienci, */
/* 		  const size_t MAX_KLIENTOW); */

extern void dodaj_klientowi_dane(void *bufor, size_t ile_danych,
				 struct sockaddr_in6 *adres,
				 klient **const klienci,
				 const size_t MAX_KLIENTOW);

extern void uaktualnij_wartosci_sitrepu(klient *const k);
#endif
