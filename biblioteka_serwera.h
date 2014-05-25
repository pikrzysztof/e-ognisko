#ifndef _BIBLIOTEKA_SERWERA_H_
#define _BIBLIOTEKA_SERWERA_H_
#include <inttypes.h>
#include <stdbool.h>

extern int16_t bezpiecznie_dodaj(const int16_t first, const int16_t second);

extern bool jest_oznaczenie(const int argc, char *const *const argv,
                            const char *const oznaczenie);


#endif
