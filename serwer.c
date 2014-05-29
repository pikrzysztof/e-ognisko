/* Trzeci program zaliczeniowy z sieci komputerowych. */
/* Napisany przez Krzysztofa Piecucha, */
/* studenta informatyki k MISMaP UW */
/* numer albumu 332534 */
/* W projekcie zostały wykorzystane fragmenty kodu z zajęć. */
/* kodowanie UTF-8 */
#include "err.h"
#include "biblioteka_serwera.h"
#include "kolejka.h"
#include "wspolne.h"
#include "bufor_wychodzacych.h"
#include "klient_struct.h"
#include <sys/socket.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <stdbool.h>
#include <assert.h>
#include <stdio.h>
#include <unistd.h>
#ifndef NDEBUG
const bool DEBUG = true;
#else
const bool DEBUG = false;
#endif

typedef struct {
	size_t rozmiar_danych;
	size_t numer_paczki;
	void *dane;
} wiadomosc;

const struct timeval czestotliwosc_raportow = {1, 0};
const size_t MAX_KLIENTOW = 20;
struct event *tcp_czytanie;
struct event *wiadomosc_na_udp;
struct event *tcp_wysylanie_raportu;
struct event_base *baza_zdarzen;
size_t liczba_klientow = 0;
klient **klienci;
wiadomosc *historia;

void udp_czytanie(evutil_socket_t gniazdo_udp, short flagi, void *nic)
{
	const size_t MTU = 2000;
	void *bufor = malloc(MTU);
	struct sockaddr adres;
	socklen_t dlugosc_adresu;
	ssize_t ile_danych = recvfrom(gniazdo_udp, bufor, MTU, MSG_DONTWAIT,
				      &adres, &dlugosc_adresu);
	if (ile_danych <= 0) {
		free(bufor);
		return;
	}
	if (ogarnij_wiadomosc_udp(bufor, ile_danych, &adres) != 0) {
		info("Przyszła wiadomość i nie udało się jej obsłużyć.\n"
		     "Treść wiadomości: ");
		if (write(STDERR_FILENO, bufor, ile_danych) < 0) {
			info("Nie udało się pokazać wiadmości.");
		}
	}
	free(bufor);
}

void czytaj_i_reaguj_tcp(evutil_socket_t gniazdo_tcp, short flagi,
			 void *dane)
{
	int32_t *numer_kliencki = (int32_t *) dane;
	struct sockaddr *addr;
	evutil_socket_t deskryptor;
	socklen_t dlugosc;
	assert(liczba_klientow < MAX_KLIENTOW);
	deskryptor = accept(gniazdo_tcp, addr, &dlugosc);
	if (deskryptor == -1) {
		info("Błąd w przyjmowaniu połączenia.");
	}
	if (wstepne_ustalenia_z_klientem(deskryptor, *numer_kliencki,
					 klienci, MAX_KLIENTOW) == 0) {
		++liczba_klientow;
	}
	++(*numer_kliencki);
	if (liczba_klientow < MAX_KLIENTOW) {
		if (event_add(tcp_czytanie, NULL) != 0) {
			perror("Nie udało się ustawić czekania na "
			       "kolejne połaczenie TCP.");
			if (liczba_klientow == 0) {
				syserr("Nie udało się ustawić czekania "
				       "na klientów to kończę.");
			}
		}
	}
	if (liczba_klientow == 1) {
		if (event_add(tcp_wysylanie_raportu, &czestotliwosc_raportow)
		    != 0) {
			syserr("Nie można wysyłać raportów!");
		}
	}
}

void wyslij_raporty(evutil_socket_t nic, short flagi, void* zero)
{
	char *raport = przygotuj_raport_grupowy(klienci, MAX_KLIENTOW);
	if (raport == NULL) {
		perror("Nie da się przygotować raportu o klientach.");
		return;
	}
	liczba_klientow += wyslij_wiadomosc_wszystkim(raport, klienci,
						      MAX_KLIENTOW, -1);
	info("Wysłałem raport %s", raport);
	free(raport);
}

int main(int argc, char **argv)
{
	const char *port;
	evutil_socket_t gniazdo_tcp;
	evutil_socket_t gniazdo_udp;
	size_t i;
	const int MAX_DLUGOSC_KOLEJKI = MAX_KLIENTOW;
	klienci = malloc(sizeof(klient *) * MAX_KLIENTOW);
	historia = malloc(sizeof(wiadomosc) * )
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
				 EV_READ & EV_PERSIST,
				 czytaj_i_reaguj_tcp, NULL);
	tcp_wysylanie_raportu = event_new(baza_zdarzen, -1,
					  EV_TIMEOUT & EV_PERSIST,
					  wyslij_raporty, NULL);
	wiadomosc_na_udp = event_new(baza_zdarzen, gniazdo_udp,
				 EV_READ & EV_PERSIST,
				 udp_czytanie, NULL);
	return EXIT_SUCCESS;
}
