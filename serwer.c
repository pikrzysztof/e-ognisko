/* Trzeci program zaliczeniowy z sieci komputerowych. */
/* Napisany przez Krzysztofa Piecucha, */
/* studenta informatyki k MISMaP UW */
/* numer albumu 332534 */
/* W projekcie zostały wykorzystane fragmenty kodu z zajęć. */
/* kodowanie UTF-8 */
#include <sys/types.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include "err.h"
#include "biblioteka_serwera.h"
#include "kolejka.h"
#include "wspolne.h"
#include "bufor_wychodzacych.h"
#include "klient_struct.h"
#ifndef NDEBUG
const bool DEBUG = true;
#else
const bool DEBUG = false;
#endif

const size_t MAX_KLIENTOW = 20;

struct do_przyjmowania_tcp {
	size_t *liczba_klientow;
	klient **const klienci;
	struct event *tcp_czytanie;
	struct event_base *baza;
};

void czytaj_i_reaguj_tcp(evutil_socket_t gniazdo_tcp, short flagi,
			 void *dane)
{
	struct do_przyjmowania_tcp *arg = (struct do_przyjmowania_tcp *)
		dane;
	struct sockaddr *addr;
	socklen_t dlugosc;
	evutil_socket_t deskryptor;
	deskryptor = accept(gniazdo_tcp, addr, &dlugosc);
	if (deskryptor == -1) {
		info("Błąd w przyjmowaniu połączenia.");
	} else {
		if (dodaj_klienta(arg->klienci, deskryptor, MAX_KLIENTOW) != 0) {
			info("Nie udało się dodać nowego klienta.");
			if (close(deskryptor) != 0)
				info("Nie udało się zamknąć deskryptora"
				     " klienta.");
		}
			close(deskryptor);

	}
}

int main(int argc, char **argv)
{
	const char *port;
	evutil_socket_t gniazdo_tcp;
	evutil_socket_t gniazdo_udp;
	size_t i, liczba_klientow = 0;
	struct event *tcp_czytanie;
	struct event *udp_czytanie;
	struct event *tcp_wysylanie_raportu;
	struct event_base *baza_zdarzen;
	const int MAX_DLUGOSC_KOLEJKI = MAX_KLIENTOW;
	klient **const klienci = malloc(MAX_KLIENTOW * sizeof(klient *));
	if (klienci == NULL)
		syserr("Nie mozna zrobic tablicy klientow.");
	ustaw_rozmiar_fifo(argc, argv);
	init_wodnego_Marka(argc, argv);
	ustaw_rozmiar_wychodzacych(argc, argv);
	port = daj_opcje(OZNACZENIE_PARAMETRU_PORTU, argc, argv);
	if (port == NULL)
		port = DOMYSLNY_NUMER_PORTU;
	else
		if (!wlasciwy_port(port))
			fatal("Jakiś lewy port.");
	gniazdo_tcp = zrob_i_przygotuj_gniazdo(port, SOCK_STREAM);
	gniazdo_udp = zrob_i_przygotuj_gniazdo(port, SOCK_DGRAM);
	for (i = 0; i < MAX_KLIENTOW; ++i) {
		i[klienci] = NULL;
	}
	baza_zdarzen = event_base_new();
	if (baza_zdarzen == NULL)
		syserr("Nie można zrobić bazy zdarzeń.");
	if (evutil_make_socket_nonblocking(gniazdo_tcp) != 0)
		syserr("Nie można zrobić gniazda TCP nieblokującego.");
	if (evutil_make_socket_nonblocking(gniazdo_udp) != 0)
		syserr("Nie można zrobić gniazda UDP nieblokującego.");
	if (listen(gniazdo_tcp, MAX_DLUGOSC_KOLEJKI) != 0)
		syserr("Nie można słuchać.");
	tcp_czytanie = event_new(baza_zdarzen, gniazdo_tcp,
				 EV_PERSIST | EV_READ, czytaj_i_reaguj_tcp,
				 klienci);

	return EXIT_SUCCESS;
}
