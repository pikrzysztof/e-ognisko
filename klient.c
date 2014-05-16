/* Trzeci program zaliczeniowy z sieci komputerowych. */
/* Napisany przez Krzysztofa Piecucha, */
/* studenta informatyki k MISMaP UW */
/* numer albumu 332534 */
/* W projekcie zostały wykorzystane fragmenty kodu z zajęć. */
/* kodowanie UTF-8 */
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include "err.h"
#include "wspolne.h"
#include <event2/event.h>
#include <event2/util.h>

#ifndef NDEBUG
static const bool DEBUG = true;
#else
static const bool DEBUG = false;
#endif

extern const char *const DOMYSLNY_NUMER_PORTU;

void zle_uzywane(const char *const nazwa_programu)
{
	fatal("Program uruchamia się %s -s nazwa_serwera", nazwa_programu);
}

typedef struct {
	int sockfd;
	int family;
} arg_keepalive;





void wyczysc_i_poczekaj(evutil_socket_t *deskryptor,
			evutil_socket_t *deskryptor_2,
			struct addrinfo **adres_binarny_serwera,
			struct event_base **baza_zdarzen)
{
	const useconds_t ile_czekac = 500000;
	if (*deskryptor != -1) {
		if (close(*deskryptor) != 0)
			perror("Nie udało się zamknąć deskryptora.\n");
		*deskryptor = -1;
	}
	if (*deskryptor_2 != -1) {
		if (close(*deskryptor_2) != 0)
			perror("Nie udało się zamknąć deskryptora.\n");
		*deskryptor_2  = -1;
	}
	if (*adres_binarny_serwera != NULL) {
		freeaddrinfo(*adres_binarny_serwera);
		*adres_binarny_serwera = NULL;
	}
	if (*baza_zdarzen != NULL) {
		event_base_free(*baza_zdarzen);
		*baza_zdarzen = NULL;
	}
	usleep(ile_czekac);
}

bool wyslij_keepalive(const int deskryptor)
{
	return wyslij_tekst(deskryptor, "KEEPALIVE\n");
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

int main(int argc, char **argv)
{
	const char* const OZNACZENIE_PARAMETRU_ADRESU = "-s";
	const char* const OZNACZENIE_PARAMETRU_PORTU = "-p";
	const char *adres_serwera = NULL;
	const char *port = NULL;
	struct addrinfo* adres_binarny_serwera = NULL;
	struct event_base *baza_zdarzen = NULL;
	evutil_socket_t deskryptor_tcp = -1;
	evutil_socket_t deskryptor_udp = -1;
	int32_t moj_numer_kliencki;
	int wstepne_flagi_stdin, wstepne_flagi_stdout;


	if (argc < 3)
		zle_uzywane(argv[0]);
	/* Nawiąż połaczenie TCP z serwerem. */
	adres_serwera = daj_opcje(OZNACZENIE_PARAMETRU_ADRESU, argc, argv);
	port = daj_opcje(OZNACZENIE_PARAMETRU_PORTU, argc, argv);
	if (adres_serwera == NULL)
		fatal("Podaj adres serwera.\n");
	if (port == NULL)
		port = DOMYSLNY_NUMER_PORTU;

	while (true) {
		wstepne_flagi_stdin = fcntl(STDIN_FILENO, F_GETFL, 0);
		wstepne_flagi_stdout = fcntl(STDOUT_FILENO, F_GETFL, 0);
		if (wstepne_flagi_stdin == -1 || wstepne_flagi_stdout == -1)
			continue;
		if (fcntl(STDIN_FILENO, F_SETFL,
			  wstepne_flagi_stdin | O_NONBLOCK) == -1) {
			perror("Nie można ustawić flag wczytywania.");
			continue;
		}
		if (fcntl(STDOUT_FILENO, F_SETFL,
			  wstepne_flagi_stdout | O_NONBLOCK) == -1) {
			perror("Nie można ustawić flag wypisywania.");
			continue;
		}
		baza_zdarzen = event_base_new();
		if (baza_zdarzen == NULL)
			continue;
		deskryptor_tcp = zrob_gniazdo(SOCK_STREAM, adres_serwera);
		if (deskryptor_tcp == -1) {
			perror("Nie można stworzyć gniazda do komunikacji TCP."
			       "\n");
			wyczysc_i_poczekaj(&deskryptor_tcp, &deskryptor_udp,
				&adres_binarny_serwera, &baza_zdarzen);
			continue;
		}
		adres_binarny_serwera = podaj_adres_binarny(adres_serwera,
							    port, IPPROTO_TCP,
							    SOCK_STREAM);
		if (adres_binarny_serwera == NULL) {
			perror("Nie można podać adresu binarnego serwera.\n");
			wyczysc_i_poczekaj(&deskryptor_tcp, &deskryptor_udp,
				&adres_binarny_serwera, &baza_zdarzen);
			continue;
		}
		if (connect(deskryptor_tcp, adres_binarny_serwera->ai_addr,
			    adres_binarny_serwera->ai_addrlen) < 0) {
			syserr("Nie można nawiązać połączenia TCP z serwerem."
			       "\n");
			wyczysc_i_poczekaj(&deskryptor_tcp, &deskryptor_udp,
				&adres_binarny_serwera, &baza_zdarzen);
			continue;
		}
		moj_numer_kliencki = odbierz_numer_kliencki(deskryptor_tcp);
		if (moj_numer_kliencki == -1) {
			perror("Serwer wysłał jakiś dziwny numer kliencki.\n");
			wyczysc_i_poczekaj(&deskryptor_tcp, &deskryptor_udp,
				&adres_binarny_serwera, &baza_zdarzen);
			continue;
		}
		freeaddrinfo(adres_binarny_serwera);
		adres_binarny_serwera = podaj_adres_binarny(adres_serwera,
							    port, IPPROTO_UDP,
							    SOCK_DGRAM);
		deskryptor_udp = zrob_gniazdo(SOCK_DGRAM, adres_serwera);
		if (deskryptor_udp == -1) {
			perror("Nie udało się zrobić deskryptora UDP.\n");
			wyczysc_i_poczekaj(&deskryptor_tcp, &deskryptor_udp,
				&adres_binarny_serwera, &baza_zdarzen);
			continue;
		}
		if (connect(deskryptor_udp, adres_binarny_serwera->ai_addr,
			    adres_binarny_serwera->ai_addrlen) < 0) {
			perror("Nie udało się połączyć po UDP.\n");
			wyczysc_i_poczekaj(&deskryptor_tcp, &deskryptor_udp,
				&adres_binarny_serwera, &baza_zdarzen);
			continue;
		}
		if (!wyslij_numer_kliencki(deskryptor_udp, moj_numer_kliencki)) {
			perror("Nie udało się zgłosić.\n");
			wyczysc_i_poczekaj(&deskryptor_tcp, &deskryptor_udp,
				&adres_binarny_serwera, &baza_zdarzen);
			continue;
		}

	}
	return EXIT_SUCCESS;
}
