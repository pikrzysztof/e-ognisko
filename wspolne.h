/* Trzeci program zaliczeniowy z sieci komputerowych. */
/* Napisany przez Krzysztofa Piecucha, */
/* studenta informatyki k MISMaP UW */
/* numer albumu 332534 */
/* W projekcie zostały wykorzystane fragmenty kodu z zajęć. */
/* kodowanie UTF-8 */
#ifndef WSPOLNE_H_
#define WSPOLNE_H_

#include <stdlib.h>
#include <stdbool.h>
#include <inttypes.h>

#define wypisz_linie debug("%i", __LINE__)

const char *const OZNACZENIE_PARAMETRU_PORTU;
const char *const DOMYSLNY_NUMER_PORTU;
const ssize_t BLAD_CZYTANIA;

typedef enum naglowek {
	CLIENT,
	UPLOAD,
	DATA,
	ACK,
	RETRANSMIT,
	KEEPALIVE,
	INNY
} rodzaj_naglowka;

extern char *const zrob_naglowek(const rodzaj_naglowka r,
				 const int32_t nr, const int32_t ack,
				 const int32_t win,
				 const size_t rozmiar_wyniku);
extern bool ustaw_gniazdo_nieblokujace(const int gniazdo);

/* Działa tylko na liczbach dodatnich. */
extern bool jest_liczba_w_przedziale(const char *const poczatek,
				     const char *const koniec,
				     const char *const liczba);

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

/* Wypisuje wiadomosc na stderr, jesli DEBUG == true */
extern void debug(const char *fmt, ...);

/* Wypisuje wiadomosci diagnostyczne na stderr. */
extern void info(const char *fmt, ...);

extern bool wyslij_numer_kliencki(int deskryptor, int32_t numer_kliencki);

extern int32_t odbierz_numer_kliencki(int deskryptor);

/* Tekst powinien być zakończony '\0' */
extern bool wyslij_tekst(int deskryptor, const char *const tekst);

/* Dopisuje na koncu napisu to, co mu kazemy. */
/* Wynik: ile bajtow wpisal lub -1 jesli blad. */
extern ssize_t dopisz_na_koncu(char *const poczatek,
			       const char *const fmt, ...);

/* Dodaje drugi napis na koncu pierwszego, konczy drugi '\0' */
extern void konkatenacja(char *const pierwszy, const char *const drugi,
			   size_t dlugosc_drugiego);

/* Czyta z deskryptora dopóki nie dostanie końca linii */
/* albo nie skończy się bufor. */
/* Jako wynik daje ile przeczytał bajtów. */
/* Jeśli wejście się skończy daje EOF. */
/* Jeśli jest bład daje BLAD_CZYTANIA. */
/* Ciag znakow zawsze konczy sie '\0', wiec wczyta o 1 mniej niz bufor. */
extern ssize_t czytaj_do_konca_linii(const int deskryptor,
				     char *const bufor,
				     const ssize_t rozmiar_bufora);

extern int max(const int a, const int b);

extern int min(const int a, const int b);

/* Czyta z deskryptora cala linie tekstu,  */
extern ssize_t czytaj_do_vectora(const int deskryptor, char **wynik);

/* Trzeba będzie ulepszyć, teraz to optymistyczna heura */
extern rodzaj_naglowka rozpoznaj_naglowek(const char *const naglowek);

/* -1 jak sie nie uda, 0 jak sie uda */
extern int wyskub_dane_z_naglowka(const char *const naglowek,
				  int32_t *const nr, int32_t *const ack,
				  int32_t *const win);

#endif
