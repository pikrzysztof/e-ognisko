#ifndef _BIBLIOTEKA_SERWERA_H_
#define _BIBLIOTEKA_SERWERA_H_
#include <inttypes.h>
#include <stdbool.h>
#include <event2/event.h>

extern int16_t bezpiecznie_dodaj(const int16_t first, const int16_t second);

extern bool jest_oznaczenie(const int argc, char *const *const argv,
                            const char *const oznaczenie);

extern evutil_socket_t zrob_i_przygotuj_gniazdo(const char *const port,
						const int protokol);


#endif
