#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <time.h>
#include <stdbool.h>
#include <stdlib.h>
#include "validate.h"


// Funkcja sprawdzająca poprawność podanych parametrów programu oraz inicjująca je
void initArgs(int argc, char* argv[]){

    // Jeśli liczba argumentów nie jest równa 3 lub 4 nazwa prorgamu, liczba czytelników, liczba pisarzy, opcjonalnie parametr "-info"
    // Wypisz informacje o błędzie na STDERR i zakończ program
    if(argc != 3 && argc != 4) 
    {
        fprintf(stderr, "Niepoprawna ilość argumentów.\n");
        fprintf(stderr, "Program przyjmuje 2 lub 3 argumenty: ilość czytelników, ilość pisarzy i (opcjonalnie -info)\n");
        exit(EXIT_FAILURE);
    }
    int j = 0;

    // Iteracja po każdym argumencie oprócz pierwszego, którym jest nazwa programu
    for(int i = 1; i < argc; i++){
        j = 0;
        // Dopóki liczba arguemntów jest mniejsza niż 3 czyli podano tylko liczbę pisarzy i liczbę czytelników
        // oraz dopóki j jest mniejsze od ilości znaków podanego argumentu sprawdzaj czy są one poprawnie skonstruowane
        while(i < 3 && j < strlen(argv[i]))
        {
            //Jeśli j-ty znak i-tego argumentu nie jest liczbą wypisz informacje na STDERR i zakończ program
            if(!isdigit(argv[i][j])){
                fprintf(stderr, "Podano niepoprawny argument.\n");
                fprintf(stderr, "Nalezy podać argumenty postaci: liczba_czytelników, liczba_pisarzy i (opcjonalnie -info)\n");
                fprintf(stderr, "Przy czym liczba czytelników i pisarzy musi być dodatnią liczbą całkowitą\n");
                exit(EXIT_FAILURE);
            }
            j++;
        }
    }

    // Jeśli argumenty są poprawne przekonwertuj je na typ int
    readers_count = atoi(argv[1]);
    writers_count = atoi(argv[2]);

    // Jeśli liczba argumentów jest równa 4 (podano parametr "-info")
    if(argc == 4){
        // Sprawdź czy napewno ostatnim parametrem jest ciąg znaków "-info"
        // W przeciwnym wypadku wypisz stosowny komunikat na STDERR i zakończ program
        if(strcmp(argv[3], "-info") != 0){
            fprintf(stderr, "Podano niepoprawny argument.\n");
            fprintf(stderr, "Nalezy podać argumenty postaci: liczba_czytelników, liczba_pisarzy i (opcjonalnie -info)\n");
            exit(EXIT_FAILURE);
        }
        // Jeśli podanym parametrem jest "-info" ustaw globalną flagę flag_info na true
        else {
            flag_info = true;
        }
    }
    
}

// Wybiera losowy czas usypiania wątków z przedziału od 1 do MAX_SLEEP_SEC
int random_sleep_time() {
    int time=rand()%MAX_SLEEP_SEC + 1;
    //int time=rand()%(MAX_SLEEP_TIME_MSEC*1000); ale wtedy usleep
    return time;
}