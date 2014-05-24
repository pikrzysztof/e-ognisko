#ifndef _BIBLIOTEKA_KLIENTA_H_
#define _BIBLIOTEKA_KLIENTA_H_
#include "wspolne.h"
#include <unistd.h>
#include <stdbool.h>
#include <string.h>
#include <event2/event.h>
#include <stdarg.h>

typedef struct {
	struct event_base* baza_zdarzen;
	ssize_t ostatnio_odebrany_nr;
	ssize_t ostatnio_odebrany_ack;
} udp_arg;

extern zrob_paczke_danych(size_t okno, int32_t nr_paczki, char **napis);

/* Pod ... sa struct event* */
extern void wyczysc(evutil_socket_t *deskryptor, evutil_socket_t *deskryptor_2,
		    struct event_base *baza_zdarzen,
		    unsigned int liczba_wydarzen, ...);

extern bool popros_o_retransmisje(const int deskryptor, const int numer);

/* Daje BLAD_CZYTANIA jak cos sie nie uda, EOF jak jest koniec pliku, */
/* 0 jak sie uda. */
extern int daj_dane_serwerowi(const int deskryptor,
			      const int numer_paczki, const size_t okno);

extern int obsluz_data(evutil_socket_t gniazdo, const char *const naglowek,
		       ssize_t *ostatnio_odebrany_ack,
		       ssize_t *ostatnio_odebrany_nr);

#endif