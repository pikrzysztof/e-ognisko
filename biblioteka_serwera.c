#include "wspolne.h"
#include "klient_struct.h"
#include "biblioteka_serwera.h"
#include "historia.h"
#include "mikser.h"
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

void odsmiecarka(klient **klienci, const size_t MAX_KLIENTOW)
{
	size_t i;
	for (i = 0; i < MAX_KLIENTOW; ++i)
		if (klienci[i] != NULL && klienci[i]->potwierdzil_numer
		    && ((clock() - klienci[i]->czas) / CLOCKS_PER_SEC) >= 1) {
			usun(klienci[i]);
			klienci[i] = NULL;
		}
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
	const evutil_socket_t gniazdo = socket(AF_INET6, typ_gniazda,
					       0);
	struct sockaddr_in6 bindowanie;
	memset(&bindowanie, 0, sizeof(bindowanie));
	bindowanie.sin6_family = AF_INET6;
	bindowanie.sin6_flowinfo = 0;
	bindowanie.sin6_port = htons(atoi(port));
	bindowanie.sin6_addr = in6addr_any;
	if (bind(gniazdo, (struct sockaddr *) &bindowanie,
		 sizeof(bindowanie))
	    != 0)
		syserr("Nie można związać gniazda z adresem i "
		       "portem.");
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

size_t zlicz_aktywnych_klientow(klient **const klienci,
				       const size_t MAX_KLIENTOW)
{
	size_t i, wynik = 0;
	for (i = 0; i < MAX_KLIENTOW; ++i)
		if (klienci[i] != NULL && klienci[i]->port != NULL)
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

void wyslij_wiadomosc_wszystkim(char *wiadomosc, klient **const klienci,
			       const size_t MAX_KLIENTOW)
{
	size_t i;
	evutil_socket_t desk;
	int wynik = 0;
	size_t rozmiar_wiadomosci = strlen(wiadomosc);
	for (i = 0; i < MAX_KLIENTOW; ++i) {
		if (klienci[i] == NULL)
			continue;
		if (wyslij_wiadomosc(wiadomosc, rozmiar_wiadomosci,
				     klienci[i], -1) == -1) {
			usun(klienci[i]);
			klienci[i] = NULL;
		}
	}
}

size_t podaj_indeks_klienta(struct sockaddr *adres,
			    klient **const klienci, const size_t MAX_KLIENTOW)
{
	size_t i;
	for (i = 0; i < MAX_KLIENTOW; ++i) {
		if (klienci[i] == NULL)
			continue;
		if (rowne((struct sockaddr *)&(klienci[i]->adres_udp),
			  adres) == 0)
			return i;
	}
	return MAX_KLIENTOW;
}

static void keepalive(struct sockaddr *adres,
		      klient **klienci, size_t MAX_KLIENTOW)
{
	size_t kto = podaj_indeks_klienta(adres, klienci, MAX_KLIENTOW);
	if (kto >= MAX_KLIENTOW)
		return;
	klienci[kto]->czas = clock();
	if (!(klienci[kto]->potwierdzil_numer)) {
		usun(klienci[kto]);
		klienci[kto] = NULL;
	}
}

void wywal(struct sockaddr* adres, klient **klienci, size_t MAX_KLIENTOW)
{
	size_t indx_klienta = podaj_indeks_klienta(adres, klienci,
						   MAX_KLIENTOW);
	if (indx_klienta >= MAX_KLIENTOW)
		return;
	usun(klienci[indx_klienta]);
	klienci[indx_klienta] = NULL;
}



static void zaktualizuj_klienta(char *bufor, size_t ile_danych,
				struct sockaddr* adres, klient **klienci,
				size_t MAX_KLIENTOW)
{
	int32_t nr, ack, win;
	size_t i;
	wyskub_dane_z_naglowka(bufor, &nr, &ack, &win);
	if (nr < 0) {
		debug("Ktoś wysłał nie taki numer co trzeba.");
		wywal(adres, klienci, MAX_KLIENTOW);
	} else {
		dodaj_adresy(nr, adres, klienci, MAX_KLIENTOW);
	}
}

static void wyslij_acka(struct sockaddr* adres, evutil_socket_t gniazdo_udp,
		   klient **klienci, size_t MAX_KLIENTOW)
{
	size_t idx_klienta = podaj_indeks_klienta(adres, klienci, MAX_KLIENTOW);
	char *odpowiedz;
	size_t rozmiar_danych = 50;
	int32_t okno;
	if (idx_klienta >= MAX_KLIENTOW) {
		info("Nie udało się znaleźć takiego klienta, co mi tu dajecie!");
		return;
	}
	okno = daj_FIFO_SIZE() -
		klienci[idx_klienta]->kolejka->liczba_zuzytych_bajtow;
	odpowiedz = zrob_naglowek(ACK,
				  klienci[idx_klienta]->spodziewany_nr_paczki,
				  -1, okno, rozmiar_danych);
	if (odpowiedz == NULL) {
		info("Nie udało się wysłać ACK, bo zabrakło pamięci.");
		return;
	}
	if (sendto(gniazdo_udp, odpowiedz, strlen(odpowiedz),
		   MSG_NOSIGNAL | MSG_DONTWAIT, adres, sizeof(*adres)) <= 0) {
		usun(klienci[idx_klienta]);
		klienci[idx_klienta] = NULL;
	}
	free(odpowiedz);
}

static int wyslij_paczke(klient *komu, evutil_socket_t gniazdo_udp,
			  int32_t numer_paczki, historia *hist)
{
	const size_t MTU = 2000;
	wpis* wpis_historii;
	size_t ile_wyslac;
	char *tmp = zrob_naglowek(DATA, numer_paczki,
				  komu->spodziewany_nr_paczki,
				  daj_win(komu->kolejka), MTU);
	wpis_historii = podaj_wpis(hist, numer_paczki);
	if (wpis_historii == NULL) {
		free(tmp);
		info("Nie udało się znaleźć w historii numeru %"SCNd32".",
		     numer_paczki);
		return 1;
	}
	ile_wyslac = strlen(tmp) + wpis_historii->dlugosc_wiadomosci;
	strncat(tmp, wpis_historii->wiadomosc,
		wpis_historii->dlugosc_wiadomosci);
	if (sendto(gniazdo_udp, tmp, ile_wyslac, MSG_DONTWAIT | MSG_NOSIGNAL,
		   (struct sockaddr *) &(komu->adres_udp),
		   sizeof(komu->adres_udp))
	    != ile_wyslac) {
		free(tmp);
		return -1;
	}
	free(tmp);
	return 0;
}

static void retransmit(char *bufor, size_t ile_danych, struct sockaddr* adres,
		       klient **klienci, size_t MAX_KLIENTOW,
		       evutil_socket_t gniazdo_udp, historia *hist)
{
	size_t idx_klienta = podaj_indeks_klienta(adres, klienci, MAX_KLIENTOW);
	char *tmp;
	size_t i, ostatnia_wiadomosc;
	int32_t nr, win, ack;
	if (hist->tablica_wpisow[hist->glowa] == NULL) {
		info("Ktoś nas poprosił o retransmisje a "
		     "my mamy pustą historię.");
		return;
	}
	if (wyskub_dane_z_naglowka(bufor, &nr, &ack, &win) != 0)
		return;
	ostatnia_wiadomosc = hist->tablica_wpisow[hist->glowa]->numer_pakietu;
	for (i = nr; i <= ostatnia_wiadomosc; ++i) {
		if (wyslij_paczke(klienci[idx_klienta], gniazdo_udp, i, hist)
		    == -1) {
			usun(klienci[idx_klienta]);
			klienci[idx_klienta] = NULL;
			return;
		}
	}
}

void ogarnij_wiadomosc_udp(char *bufor, size_t ile_danych,
			   struct sockaddr* adres, klient **klienci,
			   size_t MAX_KLIENTOW, evutil_socket_t gniazdo_udp,
			   historia *hist)
{
	bufor[ile_danych] = '\0';
	switch (rozpoznaj_naglowek(bufor)) {
	case CLIENT:
		zaktualizuj_klienta(bufor, ile_danych, adres,
					   klienci, MAX_KLIENTOW);
		break;
	case UPLOAD:
		dodaj_klientowi_dane(bufor, ile_danych, adres,
					    klienci, MAX_KLIENTOW);
		wyslij_acka(adres, gniazdo_udp, klienci, MAX_KLIENTOW);
		break;
	case RETRANSMIT:
		retransmit(bufor, ile_danych, adres,
			   klienci, MAX_KLIENTOW, gniazdo_udp, hist);
		break;
	case KEEPALIVE:
		keepalive(adres, klienci, MAX_KLIENTOW);
		break;
	default:
		wywal(adres, klienci, MAX_KLIENTOW);
		break;
	}
}

unsigned int daj_tx_interval(const int argc, char *const *const argv)
{
	const char *const OZNACZENIE = "-i";
	const char *const MIN = "1";
	const char *const MAX = "8";
	const char *tmp = daj_opcje(OZNACZENIE, argc, argv);
	const int DOMYSLNIE = 5;
	if (tmp == NULL)
		return DOMYSLNIE;
	if (jest_liczba_w_przedziale(MIN, MAX, tmp))
		return atoi(tmp);
	else
		syserr("%s powinno mieć wartości pomiędzy %s a %s.",
		       OZNACZENIE, MIN, MAX);
}

struct mixer_input* przygotuj_dane_mikserowi(klient **klienci,
				    const size_t MAX_LICZBA_KLIENTOW)
{
	size_t i, aktywni;
	struct mixer_input *inputs = malloc(MAX_LICZBA_KLIENTOW *
				      sizeof(struct mixer_input));
	if (inputs == NULL)
		syserr("Nie można zrobić tablicy dla miksera.");
	for (i = 0, aktywni = 0; i < MAX_LICZBA_KLIENTOW; ++i)
		if (klienci[i] != NULL &&
		    klienci[i]->potwierdzil_numer &&
		    klienci[i]->kolejka->stan == ACTIVE) {
			inputs[aktywni].data =
				klienci[i]->kolejka->kolejka;
			inputs[aktywni].len =
			   klienci[i]->kolejka->liczba_zuzytych_bajtow;
			++aktywni;
		}
	return inputs;
}

void wyslij_wiadomosci(const void *const dane, const size_t ile_danych,
		       const evutil_socket_t gniazdo_udp,
		       klient **const klienci,
		       const size_t MAX_KLIENTOW,
		       int32_t numer_paczki)
{
	size_t i, rozmiar_paczki;
	char *tmp;
	const size_t MTU = 2000;
	for (i = 0; i < MAX_KLIENTOW; ++i) {
		if (klienci[i] == NULL ||
		    !klienci[i]->potwierdzil_numer)
			continue;
		tmp = zrob_naglowek(DATA, numer_paczki,
				    klienci[i]->spodziewany_nr_paczki,
				    daj_win(klienci[i]->kolejka),
				    MTU);
		if (tmp == NULL)
			syserr("Zabrakło pamięci.");
		rozmiar_paczki = strlen(tmp) + ile_danych;
		strncat(tmp, dane, ile_danych);
		if (sendto(gniazdo_udp, tmp, rozmiar_paczki,
			   MSG_NOSIGNAL | MSG_DONTWAIT,
			  (struct sockaddr *) &(klienci[i]->adres_udp),
			   sizeof(klienci[i]->adres_udp))
		    != rozmiar_paczki) {
			info("Nie udało się wysłać do klienta %"
			       SCNd32", więc go usuwamy.",
			     klienci[i]->numer_kliencki);
				usun(klienci[i]);
			klienci[i] = NULL;
		}
		free(tmp);
	}
}
