#include "wspolne.h"
#include "klient_struct.h"
#include "biblioteka_serwera.h"
#include <time.h>
#include <inttypes.h>
#include <stdbool.h>
#include <event2/event.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>

/* Jeśli miałoby nastąpić przepełnienie daje wartość maksymalną/minimalną typu. */
int16_t bezpiecznie_dodaj(const int16_t pierwszy, const int16_t drugi)
{
	const int16_t wynik = pierwszy + drugi;
	if (pierwszy < 0 && drugi < 0 && (wynik > pierwszy || wynik > drugi))
		return INT16_MIN;
	if (pierwszy > 0 && drugi > 0 && (wynik < pierwszy || wynik < drugi))
		return INT16_MAX;
	return wynik;
}

bool jest_oznaczenie(const int argc, char *const *const argv,
		     const char *const oznaczenie)
{
	int i;
        for (i = 0; i < argc; ++i) {
                if (strcmp(oznaczenie, i[argv]) == 0)
                        return true;
        }
        return false;
}

evutil_socket_t zrob_i_przygotuj_gniazdo(const char *const port,
					 const int typ_gniazda)
{
	const evutil_socket_t gniazdo = socket(AF_INET6, typ_gniazda, 0);
	struct sockaddr_in6 bindowanie;
	bindowanie.sin6_family = AF_INET6;
	bindowanie.sin6_flowinfo = 0;
	bindowanie.sin6_port = htons(atoi(port));
	bindowanie.sin6_addr = in6addr_any;
	if (bind(gniazdo, (struct sockaddr *) &bindowanie, sizeof(bindowanie))
	    != 0) {
		syserr("Nie można związać gniazda z adresem i portem.");
	}
	return gniazdo;
}

int wstepne_ustalenia_z_klientem(const evutil_socket_t deskryptor_tcp,
				 const int32_t numer_kliencki,
				 klient **const klienci,
				 const size_t MAX_KLIENTOW)
{
	if (dodaj_klienta(deskryptor_tcp, numer_kliencki, klienci,
			  MAX_KLIENTOW) != 0) {
		info("Nie udało się dodać nowego klienta.");
		if (close(deskryptor_tcp) != 0)
			info("Nie udało się zamknąć deskryptora"
			     " klienta %"SCNd32".", numer_kliencki);
		return -1;
	}
	if (!wyslij_numer_kliencki(deskryptor_tcp,
				   numer_kliencki)) {
		info("Nie udało się wysłać "
		     "numeru klienckiego.");
		usun_klienta(deskryptor_tcp, klienci,
			     MAX_KLIENTOW);
		return -1;
	}
}

static size_t zlicz_aktywnych_klientow(klient **const klienci,
				       const size_t MAX_KLIENTOW)
{
	size_t i, wynik = 0;
	for (i = 0; i < MAX_KLIENTOW; ++i)
		if (klienci[i] != NULL)
			++wynik;
	return wynik;
}

char* przygotuj_raport_grupowy(klient **const klienci,
			       const size_t MAX_KLIENTOW)
{
	size_t i, ile_klientow;
	char *wynik;
	char *tmp;
	const size_t ROZMIAR_SITREPU = 100;
	ile_klientow = zlicz_aktywnych_klientow(klienci, MAX_KLIENTOW);
	if (ile_klientow == 0)
		return NULL;
	wynik = malloc(ile_klientow * ROZMIAR_SITREPU + 5);
	if (wynik == NULL) {
		syserr("Zabrakło pamięci na "
		       "wygenerowanie wiadomosci o klientach.");
	}
	wynik[0] = '\n';
	wynik[1] = '\0';
	for (i = 0; i < MAX_KLIENTOW; ++i) {
		if (klienci[i] == NULL)
			continue;
		tmp = SITREP(klienci[i]);
		if (tmp == NULL) {
			info("Nie możemy przygotować "
			     "SITREP o kliencie %"SCNd32".",
			     klienci[i]->numer_kliencki);
			continue;
		}
		strcat(wynik, tmp);
		free(tmp);
	}
	return wynik;
}

static int wyslij_wiadomosc(char *wiadomosc, size_t rozmiar, klient *kli,
			    evutil_socket_t deskryptor)
{
	if (kli == NULL)
		return 0;
	if (deskryptor == -1) {
		if (write(kli->deskryptor_tcp, wiadomosc, rozmiar) != rozmiar) {
			usun(kli);
			info("Usunęliśmy klienta bo słabo działał.");
			return -1;
		}
	} else {
		if (kli->port == NULL)
			return 0;
		if (sendto(deskryptor, wiadomosc, rozmiar,
			   MSG_DONTWAIT | MSG_NOSIGNAL,
			   (struct sockaddr *) &(kli->adres_udp),
			   sizeof(struct sockaddr_in6)) != rozmiar) {
			usun(kli);
			info("Usunęliśmy klienta bo słabo działał.");
			return -1;
		}
	}
	return 0;
}

int wyslij_wiadomosc_wszystkim(char *wiadomosc, klient **const klienci,
			       const size_t MAX_KLIENTOW,
			       const evutil_socket_t deskryptor)
{
	size_t i;
	evutil_socket_t desk;
	int wynik = 0;
	size_t rozmiar_wiadomosci = strlen(wiadomosc);
	for (i = 0; i < MAX_KLIENTOW; ++i) {
		if (klienci[i] == NULL)
			continue;
		if (wyslij_wiadomosc(wiadomosc, rozmiar_wiadomosci,
				     klienci[i], deskryptor) == -1) {
			klienci[i] = NULL;
			--wynik;
		}
	}
	return wynik;
}

static size_t podaj_indeks_klienta(struct sockaddr *adres,
				   klient **klienci, size_t MAX_KLIENTOW)
{
	size_t i;
	for (i = 0; i < MAX_KLIENTOW, ++i) {
		if (klienci[i] == NULL)
			continue;
		if (memcmp(adres, klienci[i]->adres_udp, sizeof(*adres)) == 0)
			return i;
	}
	return MAX_KLIENTOW;
}

static void keepalive(struct sockaddr *adres,
		      klient **klienci, size_t MAX_KLIENTOW)
{
	size_t kto = podaj_adres_klienta(adres, klienci, MAX_KLIENTOW);
	if ((clock() - klienci[kto]->czas) / CLOCKS_PER_SEC < 1)
		klienci[kto]->czas = clock();
}

int ogarnij_wiadomosc_udp(char *bufor, size_t ile_danych,
			  struct sockaddr* adres, klient **klienci,
			  size_t MAX_KLIENTOW)
{
	bufor[ile_danych] = '\0';
	switch (rozpoznaj_naglowek(naglowek)) {
	case CLIENT:
		return zaktualizuj_klienta(bufor, ile_danych, adres,
					   klienci, MAX_KLIENTOW);
	case UPLOAD:
		return dodaj_klientowi_dane(bufor, ile_danych, adres,
					    klienci, MAX_KLIENTOW);
	case RETRANSMIT:
		return retransmit(bufor, ile_danych, adres,
				  klienci, MAX_KLIENTOW);
	case KEEPALIVE:
		return keepalive(adres, klienci, MAX_KLIENTOW);
	default:
		return wywal(adres, klienci, MAX_KLIENTOW);
	}
}
