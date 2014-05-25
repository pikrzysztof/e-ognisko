#
# Trzeci program zaliczeniowy z sieci komputerowych.
# Napisany w przez Krzysztofa Piecucha,
# studenta II roku informatyki k. MISMaP UW.
# Numer albumu: 332534.
# W projekcie został użyty kod z laboratorium udostępniony na stronie
# www.mimuw.edu.pl/~mengel/SO/PUBLIC-SO/
# dnia 15 XI 2013.
#

# Ten Makefile jest uniwersalny, kompiluje programy nie majac w sobie zapisanej
# ich nazwy.
# Funkcje:
# $(shell polecenie) powoduje wykonanie polecenia i wpisanie jego wyjscia w
#		     w miejsce wywolania tej funkcji
# $(subst co, na-co, gdzie) zamienia wystapienia napisu "co" na napis "na-co"
#		     w tekscie "gdzie"
# $(filter-out slowa, skad) usuwa wystapienia slow wystapujacych na liscie
#		     "slowa" z tekstu "skad"
# $(wildcard wzorzec) wartoscia funkcji jest lista plikow pasujaca do wzorca.
#
# Pomysl polega na tym, aby polecic kompilatorowi odnalezienie zaleznosci
# plikow zrodlowych od plikow naglowkowych.  Wykonuje to polecenie
# gcc -MM nazwa_pliku.c.  Z tych zaleznosci tworzone sa submakefiles (*.d) dla
# kazdego pliku zrodlowego.  Owe submakefiles sa dolaczane na koncu tego
# pliku (dyrektywa include).
#
# Przed poleceniem mozemy postawic @ i wtedy to polecenie nie bedzie wypisywane
# na konsole, przed wykonaniem (inne sa wypisywane).  Podobnie, mozemy umiescic
# przed poleceniem minus i wtedy ewentualne bledy spowodowane przez to
# polecenie beda ignorowane (normalnie, status powrotu programu rozny od
# zera powoduje przerwanie dzialania make).
#
# Zmienne,a na ktore podstawiamy za pomoca := sa zmiennymi gorliwymi, tzn.
# od razu wyliczaja swoja wartosc.  Podstawienie poprzez = daje nam zmienna
# leniwa, ktora w miejscu gdzie zostanie wykorzystana wstawi swa definicje
# zywcem.

CC           := gcc
CDEBUGFLAGS  := -ggdb3 -D _DEBUG -O0
LDDEBUGFLAGS := -ggdb3 -D _DEBUG -O0
CRELEASEFLAGS:= -D NDEBUG -O2
LDRELEASEFLAGS:= -D NDEBUG -O2
SOURCES      := $(wildcard *.c)
COMMONLDFLAGS := -levent -pthread
COMMONCFLAGS := -c
COMMONFLAGS := -Wall -Wextra -pedantic -std=c99 -Wuseless-cast -Wzero-as-null-pointer-constant -Wcast-align -Wcast-qual -Wformat=2 -Winit-self -Wlogical-op -Wmissing-include-dirs -Wredundant-decls -Wshadow -Wsign-conversion -Wswitch-default -Wundef

ifeq ($(debuglevel), 1)
    LDFLAGS = $(LDDEBUGFLAGS)
    CFLAGS  = $(CDEBUGFLAGS)
else
    LDFLAGS = $(LDRELEASEFLAGS)
    CFLAGS  = $(CRELEASEFLAGS)
endif

# pliki zrodlowe
MAINOBJECTS := $(subst .c,.o,$(shell grep -l main $(SOURCES)))
# pliki obiektowe, ktore zawieraja definicje funkcji main
ALL         := $(subst .o,,$(MAINOBJECTS))
# pliki wykonywalne (powstaja ze zrodel zawierajacych definicje main)
DEPENDS     := $(subst .c,.d,$(SOURCES))
# makefefiles dla kazdego z plikow zrodlowych
ALLOBJECTS  := $(subst .c,.o,$(SOURCES))
# wszystkie pliki obiektowe
OBJECTS	    := $(filter-out $(MAINOBJECTS),$(ALLOBJECTS))
# pliki obiektowe, ktore nie zawieraja definicji main

all: $(DEPENDS) $(ALL) Makefile tags

$(DEPENDS) : %.d : %.c				# tworzenie submakefiles
	@$(CC) -MM $< > $@			# 	- zaleznosc
	@echo -e: "\t"$(CC) $(CFLAGS) $(COMMONCFLAGS) $(COMMONFLAGS) $< >> $@	#	- polecenie kompilacji

$(ALL) : % : %.o $(OBJECTS)			# konsolidacja
	@$(CC) $(LDFLAGS) $(COMMONLDFLAGS) $(COMMONFLAGS) -o $@ $^

-include $(DEPENDS)				# dolaczenie submakefiles

clean:
	-rm -f *.o $(ALL) $(ALLOBJECTS) $(DEPENDS) *~

remake: clean all

debug: $(DEPENDS) $(ALL)
	make debuglevel=1

tags: $(ALL)
	etags *.[ch]

tests:
	rm -f test
	make test debuglevel=1
	./test
