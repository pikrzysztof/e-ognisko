#ifndef _BIBLIOTEKA_SERWERA_H_
#define _BIBLIOTEKA_SERWERA_H_
#include <inttypes.h>
#include <stdbool.h>
#include <event2/event.h>
#include "historia.h"
#include "klient_struct.h"
extern int16_t bezpiecznie_dodaj(const int16_t first,
				 const int16_t second);

extern bool jest_oznaczenie(const int argc, char *const *const argv,
                            const char *const oznaczenie);

extern evutil_socket_t zrob_i_przygotuj_gniazdo(const char *const port,
						const int protokol);

extern int wstepne_ustalenia_z_klientem(
				  const evutil_socket_t deskryptor_tcp,
					const int32_t numer_kliencki,
					klient **const klienci,
					const size_t MAX_KLIENTOW);

extern char* przygotuj_raport_grupowy(klient **const klienci,
				      const size_t MAX_KLIENTOW);

/* Jak deskryptor jest -1 to wysyla po UDP. */
extern void wyslij_wiadomosc_wszystkim(char *wiadomosc,
				       klient **const klienci,
				       const size_t MAX_KLIENTOW);

extern void ogarnij_wiadomosc_udp(char *bufor, size_t ile_danych,
				  struct sockaddr_in6 *adres,
				  klient **klienci,
				  size_t MAX_KLIENTOW,
				  evutil_socket_t gniazdo_udp,
				  historia *hist);

extern size_t podaj_indeks_klienta(struct sockaddr_in6 *adres,
				   klient **const klienci,
				   const size_t MAX_KLIENTOW);

extern size_t zlicz_aktywnych_klientow(klient **const klienci,
				       const size_t MAX_KLIENTOW);

extern unsigned int daj_tx_interval(const int argc,
				    char *const *const argv);

extern struct mixer_input* przygotuj_dane_mikserowi(klient **klienci,
				    const size_t MAX_LICZBA_KLIENTOW);

extern void odsmiecarka(klient **klienci, const size_t MAX_KLIENTOW);

extern void wyslij_wiadomosci(const void *const dane, const size_t ile_danych,
			      const evutil_socket_t gniazdo_udp,
			      klient **const klienci, size_t MAX_KLIENTOW,
			      int32_t numer_paczki);
#endif
