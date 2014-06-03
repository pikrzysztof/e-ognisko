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
#include "mikser.h"
#include "historia.h"
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

const struct timeval czestotliwosc_raportow = {1, 0};
struct timeval czestotliwosc_danych;
const size_t MAX_KLIENTOW = 20;
struct event *tcp_czytanie;
struct event *wiadomosc_na_udp;
struct event *tcp_wysylanie_raportu;
struct event *udp_wysylanie_danych;
struct event_base *baza_zdarzen;
size_t liczba_klientow = 0;
klient **klienci;
historia *hist;
unsigned long TX_INTERVAL;

void zakolejkuj(struct event *zdarzenie, short flagi,
		const struct timeval *const czas,
		const char *const errmsg)
{
	if (!event_pending(zdarzenie, flagi, NULL)) {
		if (event_add(zdarzenie, czas) != 0) {
			syserr(errmsg);
		}
	}
}

void wykolejkuj(struct event *zdarzenie, const char *const errmsg)
{
	if (event_del(zdarzenie) != 0)
		syserr(errmsg);
}

void sprawdz_klientow()
{
	size_t i, polaczeni, aktywni;
	for (i = 0, polaczeni = 0, aktywni = 0; i < MAX_KLIENTOW;
	     ++i) {
		if (klienci[i] == NULL)
			continue;
		if (klienci[i]->potwierdzil_numer)
			++polaczeni;
		else
			++aktywni;
	}
	if (aktywni > 0) {
		zakolejkuj(udp_wysylanie_danych, EV_TIMEOUT,
			   &czestotliwosc_danych,
			   "Nie udało się zakolejkować "
			   "wysyłania po UDP.");
	} else {
		wykolejkuj(udp_wysylanie_danych,
			   "Nie udało się wykolejkować wysyłania "
			   "danych po UDP.");
	}
	if (polaczeni + aktywni > 0)
		zakolejkuj(tcp_wysylanie_raportu, EV_TIMEOUT,
			   &czestotliwosc_raportow,
			   "Nie udało się zakolejkować wysyłania"
			   " raportów.");
	else {
		wykolejkuj(tcp_wysylanie_raportu,
			   "Nie udało się wykolejkować wysyłania "
			   "raportów po TCP.");
	}
	if (polaczeni + aktywni < MAX_KLIENTOW) {
		zakolejkuj(tcp_czytanie, EV_READ, NULL,
			   "Nie udało się zakolejkować oczekiwania na"
			   " połaczneie TCP.");
	} else {
		wykolejkuj(tcp_czytanie, "Nie udało się wykolejkować "
			   "przyjmowania nowych połączeń.");
	}
	odsmiecarka(klienci, MAX_KLIENTOW);
}

void wyslij_dane_udp(evutil_socket_t nic, short flagi,
		     void *gniazdo_udp)
{
	static int32_t numer_paczki;
	struct mixer_input *inputs = przygotuj_dane_mikserowi(klienci,
							 MAX_KLIENTOW);
	size_t aktywni = zlicz_aktywnych_klientow(klienci,
						  MAX_KLIENTOW);
	const size_t ILE_DANYCH = 176 * TX_INTERVAL;
	void *wynik = malloc(ILE_DANYCH);
	size_t dlugosc_wyniku;
	if (wynik == NULL)
		syserr("Zabrakło pamięci.");
	mixer(inputs, aktywni, wynik, &dlugosc_wyniku, TX_INTERVAL);
	odejmij_ludziom(klienci, inputs, MAX_KLIENTOW);
	wyslij_wiadomosci(wynik, dlugosc_wyniku,
		    *((evutil_socket_t *) gniazdo_udp), klienci,
		    MAX_KLIENTOW, numer_paczki);
	dodaj_wpis(hist, numer_paczki, wynik, dlugosc_wyniku);
	++numer_paczki;
}

void udp_czytanie(evutil_socket_t gniazdo_udp, short flagi, void *nic)
{
	const size_t MTU = 2000;
	void *bufor = malloc(MTU);
	struct sockaddr adres;
	socklen_t dlugosc_adresu;
	ssize_t ile_danych = recvfrom(gniazdo_udp, bufor, MTU,
				      MSG_DONTWAIT,
				      &adres, &dlugosc_adresu);
	if (ile_danych <= 0) {
		syserr("Recv po udp się nie powiodło.");
	}
	ogarnij_wiadomosc_udp(bufor, ile_danych, &adres, klienci,
			      MAX_KLIENTOW, gniazdo_udp, hist);
	free(bufor);
	sprawdz_klientow();
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
	sprawdz_klientow();
}

void wyslij_raporty(evutil_socket_t nic, short flagi, void* zero)
{
	char *raport = przygotuj_raport_grupowy(klienci, MAX_KLIENTOW);
	if (raport == NULL) {
		perror("Nie da się przygotować raportu o klientach.");
		return;
	}
	wyslij_wiadomosc_wszystkim(raport, klienci, MAX_KLIENTOW);
	info("Wysłałem raport %s", raport);
	free(raport);
	sprawdz_klientow();
}

int main(int argc, char **argv)
{
	const char *port;
	evutil_socket_t gniazdo_tcp;
	evutil_socket_t gniazdo_udp;
	size_t i;
	const int MAX_DLUGOSC_KOLEJKI = MAX_KLIENTOW;
	klienci = malloc(sizeof(klient *) * MAX_KLIENTOW);
	hist = historia_init(argc, argv);
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
	TX_INTERVAL = daj_tx_interval(argc, argv);
	czestotliwosc_danych.tv_sec = 0;
	czestotliwosc_danych.tv_usec = TX_INTERVAL * 1000;
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
	udp_wysylanie_danych = event_new(baza_zdarzen, -1,
				       EV_PERSIST & EV_TIMEOUT,
				       wyslij_dane_udp, &gniazdo_udp);
	if (wyslij_dane_udp == NULL || tcp_czytanie == NULL ||
	    tcp_wysylanie_raportu == NULL ||
	    udp_wysylanie_danych == NULL)
		syserr("Nie udało się zrobić zdarzeń.");
	sprawdz_klientow();
	if (event_base_dispatch(baza_zdarzen) != 0)
		syserr("Nie udało się uruchomić zdarzeń.");
	return EXIT_SUCCESS;
}
