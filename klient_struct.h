#ifndef _KLIENT_STRUCT_H
#define _KLIENT_STRUCT_H
#include "kolejka.h"
#include <event2/event.h>
typedef struct {
	FIFO *kolejka;
	char *adres;
	char *port;
	size_t min_rozmiar_ostatnio;
	size_t max_rozmiar_ostatnio;
	evutil_socket_t deskryptor_tcp;
	in_port_t sin_port;
	struct sockaddr_in6 adres_udp;
	int32_t numer_kliencki;
} klient;

extern const char *SITREP(const klient *const o_kim);

extern void usun(klient *kto);

extern int dodaj_klienta(const evutil_socket_t deskryptor,
			 const int32_t numer_kliencki,
			 klient **const klienci,
			 const size_t dlugosc_tablicy);

extern void usun_klienta(const evutil_socket_t deskryptor_tcp,
			 klient **const klienci,
			 const size_t dlugosc_tablicy);

#endif
