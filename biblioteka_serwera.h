#ifndef _BIBLIOTEKA_SERWERA_H_
#define _BIBLIOTEKA_SERWERA_H_
#include <inttypes.h>
#include <stdbool.h>
#include <event2/event.h>
#include "klient_struct.h"
extern int16_t bezpiecznie_dodaj(const int16_t first, const int16_t second);

extern bool jest_oznaczenie(const int argc, char *const *const argv,
                            const char *const oznaczenie);

extern evutil_socket_t zrob_i_przygotuj_gniazdo(const char *const port,
						const int protokol);

extern int wstepne_ustalenia_z_klientem(const evutil_socket_t deskryptor_tcp,
					const int32_t numer_kliencki,
					klient **const klienci,
					const size_t MAX_KLIENTOW);

extern char* przygotuj_raport_grupowy(klient **const klienci,
				      const size_t MAX_KLIENTOW);

#endif
