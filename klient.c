/* Trzeci program zaliczeniowy z sieci komputerowych. */
/* Napisany przez Krzysztofa Piecucha, */
/* studenta informatyki k MISMaP UW */
/* numer albumu 332534 */
/* W projekcie zostały wykorzystane fragmenty kodu z zajęć. */
/* kodowanie UTF-8 */
#include <unistd.h>
#include <stdbool.h>
#include <string.h>
#include "err.h"
#include "wspolne.h"
#include <event2/event.h>

#ifndef NDEBUG
static const bool DEBUG = true;
#else
static const bool DEBUG = false;
#endif

bool blad;

void zle_uzywane(const char *const nazwa_programu)
{
	fatal("Program uruchamia się %s -s nazwa_serwera", nazwa_programu);
}

void wyczysc(evutil_socket_t *deskryptor,
	     evutil_socket_t *deskryptor_2,
	     struct event_base *baza_zdarzen,
	     struct event* wydarzenie)
{
	if (deskryptor != NULL) {
		if (close(*deskryptor) != 0)
			perror("Nie udało się zamknąć deskryptora.");
	}
	if (deskryptor_2 != NULL) {
		if (close(*deskryptor_2) != 0)
			perror("Nie udało się zamknąć deskryptora.");
	}
	if (wydarzenie != NULL) {
		if (event_del(wydarzenie) != 0)
			perror("Nie udało się wykasować wydarzenia.");
	}
	if (baza_zdarzen != NULL) {
		event_base_free(baza_zdarzen);
	}
}

void wyslij_keepalive(evutil_socket_t minus_jeden, short flagi,
		      void *deskryptor)
{
	const int arg = *((int *) deskryptor);
	if (!wyslij_tekst(arg, "KEEPALIVE\n")) {
		perror("Nie udalo sie wyspac KEEPALIVE.");
		blad = true;
	}
}

bool popros_o_retransmisje(const int deskryptor, const int numer)
{
	const size_t MAX_ROZMIAR_BUFORA = 30;
	char *const tekst = malloc(MAX_ROZMIAR_BUFORA * sizeof(char));
	bool wynik;
	if (tekst == NULL)
		return false;
	if (sprintf(tekst, "RETRANSMIT %i\n", numer) < 0) {
		free(tekst);
		return false;
	}
	wynik = wyslij_tekst(deskryptor, tekst);
	free(tekst);
	return wynik;
}

/* Daje BLAD_CZYTANIA jak cos sie nie uda, EOF jak jest koniec pliku, */
/* 0 jak sie uda. */
int daj_dane_serwerowi(const int deskryptor,
			const int numer_paczki, const size_t okno)
{
	const char *const POCZATEK_KOMUNIKATU = "UPLOAD ";
	size_t DLUGOSC_POCZATKU = strlen(POCZATEK_KOMUNIKATU) + 10;
	char *const tekst = malloc(okno + DLUGOSC_POCZATKU);
	ssize_t ile_przeczytane;
	if (tekst == NULL)
		return BLAD_CZYTANIA;
	if (sprintf(tekst, "%s%i\n", POCZATEK_KOMUNIKATU, numer_paczki) < 0) {
		free(tekst);
		return BLAD_CZYTANIA;
	}
	ile_przeczytane = read(deskryptor, tekst + strlen(tekst), okno);
	if (ile_przeczytane < 0) {
		free(tekst);
		return BLAD_CZYTANIA;
	}
	if (ile_przeczytane == 0) {
		free(tekst);
		return EOF;
	}
	if (wyslij_tekst(deskryptor, tekst)) {
		free(tekst);
		return 0;
	}
	free(tekst);
	return BLAD_CZYTANIA;
}

evutil_socket_t ustanow_polaczenie(const int protokol,
				   const char* const adres_serwera,
				   const char* const port)
{
	const int typ_gniazda = protokol == IPPROTO_TCP ?
                                            SOCK_STREAM : SOCK_DGRAM;
	const evutil_socket_t gniazdo = zrob_gniazdo(typ_gniazda,
						     adres_serwera);
	struct addrinfo* adres_binarny_serwera = NULL;
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
}

void dzialaj(const char* const adres_serwera, const char* const port)
{
	struct event* wysylanie_keepalive;
	struct event_base *baza_zdarzen = NULL;
	evutil_socket_t deskryptor_tcp = -1;
	evutil_socket_t deskryptor_udp = -1;
	int32_t moj_numer_kliencki;
	struct timeval dziesiec_setnych = {0, 100000};
	debug("ustawiamy gniazda na nieblokujace");
	if (!ustaw_gniazdo_nieblokujace(STDIN_FILENO) ||
	    !ustaw_gniazdo_nieblokujace(STDOUT_FILENO)) {
		perror("Nie mozna ustawic STDIN/STDOUT na nieblokujace.");
		return;
	}
	debug("ustawilismy gniazda na nieblokujace");
	deskryptor_tcp = ustanow_polaczenie(IPPROTO_TCP, adres_serwera, port);
	if (deskryptor_tcp == -1) {
		perror("Nie można ustanowić połączenia TCP.");
		return;
	}
	moj_numer_kliencki = odbierz_numer_kliencki(deskryptor_tcp);
	if (moj_numer_kliencki == -1) {
		perror("Serwer wysłał jakiś dziwny numer kliencki.");
		wyczysc(&deskryptor_tcp, NULL, NULL, NULL);
		return;
	}
	deskryptor_udp = ustanow_polaczenie(IPPROTO_UDP, adres_serwera, port);
	if (deskryptor_udp == -1) {
		perror("Nie można ustanowić połączenia UDP.");
		wyczysc(&deskryptor_tcp, NULL, NULL, NULL);
		return;
	}
	if (!wyslij_numer_kliencki(deskryptor_udp, moj_numer_kliencki)) {
		perror("Nie udało się zgłosić.\n");
		wyczysc(&deskryptor_tcp, &deskryptor_udp, NULL, NULL);
		return;
	}
	baza_zdarzen = event_base_new();
	if (baza_zdarzen == NULL) {
		wyczysc(&deskryptor_tcp, &deskryptor_udp, NULL, NULL);
		return;
	}
	wysylanie_keepalive = event_new(baza_zdarzen, -1, EV_PERSIST,
					wyslij_keepalive, &deskryptor_udp);
	if (event_add(wysylanie_keepalive, &dziesiec_setnych) != 0) {
		wyczysc(&deskryptor_tcp, &deskryptor_udp, baza_zdarzen,
			wysylanie_keepalive);
		return;
	}
	debug("Pętla obsługi zdarzeń!");
	if (event_base_dispatch(baza_zdarzen) == -1) {
		perror("Nie udało się uruchomić pętli obsługi zdarzeń.");
		wyczysc(&deskryptor_tcp, &deskryptor_udp, baza_zdarzen,
			wysylanie_keepalive);
		return;
	}
	debug("Wychodzimy z pętli obsługi zdarzeń.");
}

int main(int argc, char **argv)
{
	const char* const OZNACZENIE_PARAMETRU_ADRESU = "-s";
	const char* const OZNACZENIE_PARAMETRU_PORTU = "-p";
	const char *adres_serwera = NULL;
	const char *port = NULL;
	const useconds_t ILE_SPAC = 500000;
	if (argc < 3)
		zle_uzywane(argv[0]);
	adres_serwera = daj_opcje(OZNACZENIE_PARAMETRU_ADRESU, argc, argv);
	port = daj_opcje(OZNACZENIE_PARAMETRU_PORTU, argc, argv);
	if (adres_serwera == NULL)
		fatal("Podaj adres serwera.\n");
	if (port == NULL)
		port = DOMYSLNY_NUMER_PORTU;
	while (true) {
		dzialaj(adres_serwera, port);
		usleep(ILE_SPAC);
	}
	return EXIT_SUCCESS;
}
