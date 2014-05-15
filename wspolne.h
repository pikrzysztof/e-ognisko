/* Trzeci program zaliczeniowy z sieci komputerowych. */
/* Napisany przez Krzysztofa Piecucha, */
/* studenta informatyki k MISMaP UW */
/* numer albumu 332534 */
/* W projekcie zostały wykorzystane fragmenty kodu z zajęć. */
/* kodowanie UTF-8 */

#include <stdlib.h>
#include <stdbool.h>
#include <inttypes.h>
#ifndef COMMON_H
#define COMMON_H

const char* const DOMYSLNY_NUMER_PORTU;

extern bool wyslij_numer_kliencki(int deskryptor, int32_t numer_kliencki);

extern int32_t odbierz_numer_kliencki(int deskryptor);

/* Tekst powinien być zakończony '\0' */
extern bool wyslij_tekst(int deskryptor, const char *const tekst);

/* Czyta z deskryptora dopóki nie dostanie końca linii */
/* albo nie skończy się bufor. */
/* Jako wynik daje ile przeczytał bajtów. */
extern int czytaj_do_konca_linii(int deskryptor,
				  char *bufor, size_t rozmiar_bufora);

/* Wypisuje wiadomosc na stderr, jesli DEBUG == true */
extern void debug(const char *fmt, ...);

/* Sprawdza, czy argument jest napisem przedstawiajacym poprawny numer portu */
extern bool wlasciwy_port(const char *const number);

/* Znajduje wpis */
extern const char* daj_opcje(const char *const nazwa_opcji,
				   const int rozmiar_tablicy_opcji,
				   char *const *const tablica_opcji);

/* Sprawdza, czy podany napis przedstawia adres IPv6 */
extern bool jest_ipv6(const char *const domniemany_adres);

/* Achtung! Trzeba zwolnić pamięć po wywołaniu tej funkcji. */
/* Daje NULL jeśli się nie powiedzie. */
extern struct addrinfo* podaj_adres_binarny(const char *const ludzki_adres,
					    const char *const port,
					    const int protokol,
					    const int typ_gniazda);

/* Tworzy odpowiednie gniazdo, samo wykrywa czy powinno użyć IPv4 czy IPv6 */
extern int zrob_gniazdo(const int typ, const char *const ludzki_adres);

/* Działa tylko na liczbach dodatnich. */
extern bool jest_liczba_w_przedziale(const char *const poczatek,
				     const char *const koniec,
				     const char *const liczba);



#endif
