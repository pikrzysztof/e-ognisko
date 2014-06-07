/* Trzeci program zaliczeniowy z sieci komputerowych. */
/* Napisany przez Krzysztofa Piecucha, */
/* studenta informatyki k MISMaP UW */
/* numer albumu 332534 */
/* W projekcie zostały wykorzystane fragmenty kodu z zajęć. */
/* kodowanie UTF-8 */
#include <assert.h>
#include <event2/event.h>
#include <netdb.h>
#include <stdarg.h>
#include <stdbool.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

#include "err.h"
#include "wspolne.h"
#include "biblioteka_klienta.h"


#ifndef NDEBUG
static const bool DEBUG = true;
#else
static const bool DEBUG = false;
#endif

void zle_uzywane(const char *const nazwa_programu)
{
	fatal("Program uruchamia się %s -s nazwa_serwera",
	      nazwa_programu);
}

void skoncz_petle(struct event_base *baza, char *komunikat)
{
	perror(komunikat);
	if (event_base_loopbreak(baza) != 0) {
		perror("Nie można wyskoczyć z pętli.");
	}
}

void czytaj_i_reaguj_tcp(evutil_socket_t gniazdo_tcp, short flagi,
			 void *baza_zdarzen)
{
	const size_t MAX_ROZMIAR = 20000;
	char *const tmp = malloc(MAX_ROZMIAR);
	ssize_t ile_przyszlo;
	if (tmp == NULL)
		syserr("Mało pamięci.");
	if (flagi & EV_TIMEOUT || !(flagi & EV_READ)) {
		skoncz_petle((struct event_base *) baza_zdarzen,
			     "Problem z połączeniem TCP, "
			     "długo nie ma wiadomości.");
		return;
	}
	ile_przyszlo = read(gniazdo_tcp, tmp, MAX_ROZMIAR);
	if (ile_przyszlo <= 0) {
		free(tmp);
		skoncz_petle((struct event_base *) baza_zdarzen,
			     "Serwer daje dziwne rzeczy.");
	} else {
		if (write(STDERR_FILENO, tmp, (size_t) ile_przyszlo)
		    != ile_przyszlo) {
			syserr("Nie da się pisać na stdout.");
		}
		free(tmp);
	}
}


void czytaj_i_reaguj_udp(evutil_socket_t gniazdo_udp, short flagi,
			 void *baza_i_liczby)
{
	udp_arg potrzebne = *((udp_arg *) baza_i_liczby);
	char *naglowek;
	ssize_t ile_wczytane;
	if (flagi & EV_TIMEOUT || !(flagi & EV_READ)) {
		skoncz_petle(potrzebne.baza_zdarzen,
			     "Za długo czekamy na UDP.");
		return;
	}
	naglowek = malloc(MTU);
	if (naglowek == NULL)
		syserr("Brakuje pamięci.");
	ile_wczytane = recv(gniazdo_udp, naglowek, MTU, MSG_DONTWAIT);
	if (ile_wczytane <= 0) {
		free(naglowek);
		skoncz_petle(potrzebne.baza_zdarzen, "Dziwny nagłówek UDP.");
		return;
	}
	naglowek[ile_wczytane] = '\0';
	switch (rozpoznaj_naglowek(naglowek)) {
	case DATA:
		if (obsluz_data(gniazdo_udp, naglowek, (size_t) ile_wczytane,
				&potrzebne.ostatnio_odebrany_ack,
				&potrzebne.ostatnio_odebrany_nr) != 0)
			skoncz_petle(potrzebne.baza_zdarzen, "Źle z DATA.");
		free(naglowek);
		break;
	case ACK:
		if (obsluz_ack(gniazdo_udp, naglowek,
			       &potrzebne.ostatnio_odebrany_ack) != 0)
			skoncz_petle(potrzebne.baza_zdarzen, "Źle z ACK.");
		free(naglowek);
		break;
	default:
		skoncz_petle(potrzebne.baza_zdarzen,
			     "Dziwny pakiet od serwera.");
		free(naglowek);
		break;
	}
}

void wyslij_keepalive(evutil_socket_t minus_jeden, short flagi,
		      void *deskryptor)
{
	const int arg = *((int *) deskryptor);
	unused(minus_jeden);
	unused(flagi);
	if (!wyslij_tekst(arg, "KEEPALIVE\n")) {
		perror("Nie udalo sie wyspac KEEPALIVE.");
	}
}

evutil_socket_t ustanow_polaczenie(const int protokol,
				   const char* const adres_serwera,
				   const char* const port)
{
	const int typ_gniazda = protokol == IPPROTO_TCP ?
                                            SOCK_STREAM : SOCK_DGRAM;
	const evutil_socket_t gniazdo = zrob_gniazdo(typ_gniazda,
						     adres_serwera);
	struct addrinfo* adres_binarny_serwera;
	if (gniazdo == -1)
		return gniazdo;
	adres_binarny_serwera = podaj_adres_binarny(adres_serwera,
						    port, protokol,
						    typ_gniazda);
	if (adres_binarny_serwera == NULL) {
		perror("Nie można pobrać adresu serwera!");
		if (close(gniazdo) < 0)
			perror("Nie można zamknąć gniazda!");
		return -1;
	}
	if (connect(gniazdo, adres_binarny_serwera->ai_addr,
		    adres_binarny_serwera->ai_addrlen) < 0) {
		perror("Nie można podłączyć się do serwera");
		freeaddrinfo(adres_binarny_serwera);
		if (close(gniazdo) < 0)
			perror("Nie można zamknąć gniazda.");
		return -1;
	}
	freeaddrinfo(adres_binarny_serwera);
	return gniazdo;
}

void dzialaj(const char* const adres_serwera, const char* const port)
{
	struct event* wysylanie_keepalive;
	struct event* wiadomosc_na_tcp = NULL;
	struct event* wiadomosc_na_udp = NULL;
	struct event_base *baza_zdarzen = NULL;
	evutil_socket_t deskryptor_tcp = -1;
	evutil_socket_t deskryptor_udp = -1;
	int32_t moj_numer_kliencki;
	udp_arg dla_funkcji_udp;
	struct timeval dziesiec_setnych = {0, 100000};
	debug("ustawiamy gniazda na nieblokujace");
	if (!ustaw_gniazdo_nieblokujace(STDIN_FILENO)) {
		perror("Nie mozna ustawic STDIN/STDOUT na"
		       " nieblokujace.");
		return;
	}
	debug("ustawilismy gniazda na nieblokujace");
	deskryptor_tcp = ustanow_polaczenie(IPPROTO_TCP,
					    adres_serwera, port);
	if (deskryptor_tcp == -1) {
		perror("Nie można ustanowić połączenia TCP.");
		return;
	}
	moj_numer_kliencki = odbierz_numer_kliencki(deskryptor_tcp);
	if (moj_numer_kliencki == -1) {
		perror("Serwer wysłał jakiś dziwny numer kliencki.");
		wyczysc(&deskryptor_tcp, NULL, NULL, 0);
		return;
	}
	if (evutil_make_socket_nonblocking(deskryptor_tcp) != 0) {
		perror("Nie da sie ustawic gniazda TCP na "
		       "nieblokujace.");
		wyczysc(&deskryptor_tcp, NULL, NULL, 0);
		return;
	}
	deskryptor_udp = ustanow_polaczenie(IPPROTO_UDP, adres_serwera,
					    port);
	if (deskryptor_udp == -1) {
		perror("Nie można ustanowić połączenia UDP.");
		wyczysc(&deskryptor_tcp, NULL, NULL, 0);
		return;
	}
	if (!wyslij_numer_kliencki(deskryptor_udp, moj_numer_kliencki)) {
		perror("Nie udało się zgłosić.\n");
		wyczysc(&deskryptor_tcp, &deskryptor_udp, NULL, 0);
		return;
	}
	debug("wyslalismy nr kliencki");
	baza_zdarzen = event_base_new();
	if (baza_zdarzen == NULL) {
		perror("Nie można zrobić bazy zdarzeń.");
		wyczysc(&deskryptor_tcp, &deskryptor_udp, NULL, 0);
		return;
	}
	wysylanie_keepalive = event_new(baza_zdarzen, -1, EV_PERSIST,
					wyslij_keepalive,
					&deskryptor_udp);
	if (event_add(wysylanie_keepalive, &dziesiec_setnych) != 0) {
		perror("Nie da się ustawić wysyłania KEEPALIVE.");
		wyczysc(&deskryptor_tcp, &deskryptor_udp, baza_zdarzen,
			1, wysylanie_keepalive);
		return;
	}
	dla_funkcji_udp.baza_zdarzen = baza_zdarzen;
	dla_funkcji_udp.ostatnio_odebrany_ack = 0;
	dla_funkcji_udp.ostatnio_odebrany_nr = -1;
	wiadomosc_na_udp = event_new(baza_zdarzen, deskryptor_udp,
				     EV_PERSIST | EV_READ, czytaj_i_reaguj_udp,
				     &dla_funkcji_udp);
	wiadomosc_na_tcp = event_new(baza_zdarzen, deskryptor_tcp,
				     EV_PERSIST | EV_READ, czytaj_i_reaguj_tcp,
				     baza_zdarzen);
	if (wiadomosc_na_tcp == NULL || wiadomosc_na_udp == NULL) {
		perror("Nie da się utworzyc wydarzenia z czytaniem.");
		wyczysc(&deskryptor_tcp, &deskryptor_udp, baza_zdarzen,
			3, wysylanie_keepalive, wiadomosc_na_tcp,
			wiadomosc_na_udp);
		return;
	}
	if (event_add(wiadomosc_na_tcp, NULL) != 0) {
		perror("Nie da się wrzucić czytania TCP.");
		wyczysc(&deskryptor_tcp, &deskryptor_udp, baza_zdarzen,
			4, wysylanie_keepalive, wiadomosc_na_tcp,
			wiadomosc_na_udp);
		return;
	}
	if (event_add(wiadomosc_na_udp, NULL) != 0) {
		perror("Nie da się wrzucić czytania TCP.");
		wyczysc(&deskryptor_tcp, &deskryptor_udp, baza_zdarzen,
			4, wysylanie_keepalive, wiadomosc_na_tcp,
			wiadomosc_na_udp);
		return;
	}
	debug("Pętla obsługi zdarzeń!");
	debug("UDP desc: %i, TCP desc: %i", deskryptor_udp, deskryptor_tcp);
	if (event_base_dispatch(baza_zdarzen) == -1) {
		perror("Nie udało się uruchomić pętli obsługi "
		       "zdarzeń.");
		wyczysc(&deskryptor_tcp, &deskryptor_udp, baza_zdarzen,
			3, wysylanie_keepalive, wiadomosc_na_tcp,
			wiadomosc_na_udp);
		return;
	}
	wyczysc(&deskryptor_tcp, &deskryptor_udp, baza_zdarzen,
		3, wysylanie_keepalive, wiadomosc_na_tcp,
		wiadomosc_na_udp);
	debug("Wychodzimy z pętli obsługi zdarzeń.");
}

int main(int argc, char **argv)
{
	const char *const OZNACZENIE_PARAMETRU_ADRESU = "-s";
	const char *adres_serwera = NULL;
	const char *port = NULL;
	const struct timespec ILE_SPAC = {0, 500000000};
	if (argc < 3)
		zle_uzywane(argv[0]);
	adres_serwera = daj_opcje(OZNACZENIE_PARAMETRU_ADRESU, argc,
				  argv);
	port = daj_opcje(OZNACZENIE_PARAMETRU_PORTU, argc, argv);
	if (adres_serwera == NULL)
		fatal("Podaj adres serwera.\n");
	if (port == NULL)
		port = DOMYSLNY_NUMER_PORTU;
	while (true) {
		dzialaj(adres_serwera, port);
		nanosleep(&ILE_SPAC, NULL);
	}
	return EXIT_SUCCESS;
}
