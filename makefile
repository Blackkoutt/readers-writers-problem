objects = validate.o list.o fifo.o ticket_lock.o # Lista plików obiektowych

executable_files = writers_starvation readers_starvation no_starvation # Lista plików wykonywalnych
objects_files =  writers_starvation.o readers_starvation.o no_starvation.o # Lista plików obiektowych

all: readers_starvation writers_starvation no_starvation # Wszystkie pliki wykonywalne

writers_starvation: writers_starvation.o $(objects) # Plik wykonywalny glod_pisarze jest tworzony z listy plikow obiektowych i glod_pisarze.o
	gcc writers_starvation.c $(objects) -o writers_starvation -pthread

readers_starvation: readers_starvation.o $(objects) # Plik wykonywalny glod_czytelnicy jest tworzony z listy plikow obiektowych i glod_czytelnicy.o
	gcc readers_starvation.c $(objects) -o readers_starvation -pthread

no_starvation: no_starvation.o $(objects) # Plik wykonywalny brak_zaglodzenia jest tworzony z listy plikow obiektowych i brak_zaglodzenia.o
	gcc no_starvation.c $(objects) -o no_starvation -pthread

validate.o: validate.c validate.h # Plik obiektowy validate.o jest tworzony z plikow validate.c i validate.h
	gcc -c validate.c -o validate.o

list.o: list.c list.h # Plik obiektowy list.o jest tworzony z plikow list.c i list.h
	gcc -c list.c -o list.o

fifo.o: fifo.c fifo.h list.h # Plik obiektowy fifo.o jest tworzony z plikow fifo.c, fifo.h i list.h
	gcc -c fifo.c -o fifo.o

ticket_lock.o: ticket_lock.c ticket_lock.h # Plik obiektowy ticket_lock.o jest tworzony z plikow ticket_lock.c i ticket_lock.h
	gcc -c ticket_lock.c -o ticket_lock.o -pthread

.PHONY: clean # Elimunuje bledy w przypadku gdy w katalogu znajduje sie takze plik o nazwie clean

#clean usuwa plik wykonywalny glod_pisarze oraz wszystkie stworzone pliki obiektowe
clean:
	rm $(executable_files) $(objects_files) $(objects)