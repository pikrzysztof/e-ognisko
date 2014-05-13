#include <stdbool.h>
#include <inttypes.h>
#ifndef COMMON_H
#define COMMON_H

struct opcje {
	bool poprawna_struktura;
	bool IPv6;
	uint16_t port;
	struct addrinfo
	 binary_address;

}

/* Sprawdza, czy argument jest napisem przedstawiajacym poprawny numer portu */
extern bool wlasciwy_port(const char *const number);

/* Znajduje wpis */
extern const char *const daj_opcje(const char *const nazwa_opcji,
			    const int rozmiar_tablicy_opcji
			    const *char *const tablica_opcji);

#endif
