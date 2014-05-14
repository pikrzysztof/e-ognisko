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
#include "err.h"
#include "wspolne.h"

#ifndef NDEBUG
const bool DEBUG = true;
#else
const bool DEBUG = false;
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

int32_t przywitaj_sie(int deskryptor_tcp)
{
	size_t max_rozmiar_wiadomosci_powitalnej = 30;
	char *przyjmowane = malloc(sizeof(char) * max_rozmiar_wiadomosci_powitalnej);
	const char *const MAX_NUMER_KLIENTA = "2147483647";
	const char *const MIN_NUMER_KLIENTA = "0";
	ssize_t tmp;
	int i = -1;
	do {
		i++;
		tmp = read(deskryptor_tcp, przyjmowane + i, 1);
	} while (przyjmowane[i] != '\n' || tmp == 0);
	if (tmp == 0)
		return -1;
	if (strcmp(przyjmowane, "CLIENT") != 0)
		return -1;
	przyjmowane[i] = '\0';
	if (!jest_liczba_w_przedziale(MIN_NUMER_KLIENTA, MAX_NUMER_KLIENTA,
				      przyjmowane + i))
	    return -1;
	return atoi(przyjmowane + i);

}

int main(int argc, char **argv)
{
	const char* const OZNACZENIE_PARAMETRU_ADRESU = "-s";
	const char* const OZNACZENIE_PARAMETRU_PORTU = "-p";
	const char *adres_serwera;
	const char *port;
	struct addrinfo* adres_binarny_serwera;
	int deskryptor_tcp;

	if (argc < 3)
		zle_uzywane(argv[0]);
	/* Nawiąż połaczenie TCP z serwerem. */
	adres_serwera = daj_opcje(OZNACZENIE_PARAMETRU_ADRESU, argc, argv);
	port = daj_opcje(OZNACZENIE_PARAMETRU_PORTU, argc, argv);
	if (adres_serwera == NULL)
		fatal("Podaj adres serwera.\n");
	if (port == NULL)
		port = DOMYSLNY_NUMER_PORTU;
	deskryptor_tcp = zrob_gniazdo(SOCK_STREAM, adres_serwera);
	if (deskryptor_tcp == -1)
		syserr("Nie można zrobić gniazda TCP.");
	adres_binarny_serwera = podaj_adres_binarny(adres_serwera,
						    port, IPPROTO_TCP,
						    SOCK_STREAM);
	if (adres_binarny_serwera == NULL)
		syserr("Nie można pobrać adresu serwera.");
	if (connect(deskryptor_tcp, adres_binarny_serwera->ai_addr,
		    adres_binarny_serwera->ai_addrlen) < 0)
		syserr("Nie można nawiązać połączenia TCP z serwerem.");
	return EXIT_SUCCESS;
}
