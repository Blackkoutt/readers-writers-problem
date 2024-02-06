#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <pthread.h>
#include <time.h>
#include <unistd.h>
#include <semaphore.h>
#include "fifo.h"
#include "list.h"
#include "validate.h"
#include "ticket_lock.h"

// Struktura parametrów przekazywanych do funkcji wykonywanej przez pisarza
typedef struct {
    FIFO* writersQueue; // Kolejka pisarzy oczekujących na wejście do czytelni
    int* writer_num;    // Numer wątku danego pisarza
}WriterThreadData;

// Struktura parametrów przekazywanych do funkcji wykonywanej przez czytelnika
typedef struct {
    FIFO* readersQueue; // Kolejka czytelników oczekujących na wejście do czytelni
    int* reader_num;    // Numer wątku danego czytelnika
}ReaderThreadData;

volatile int readers_count = 0; // Liczba czytelników podana jako parametr programu
volatile bool writer_enter = true; // Flaga informująca czy pisarz jest w czytelni
volatile int writers_count = 0; // Liczba pisarzy podana jako parametr programu
volatile bool flag_info = false;    // Flaga wskazująca na to czy został podany parametr "-info"
volatile int writers_in_reading_room = 0;   // Liczba pisarzy aktualnie przebywających w czytelni
volatile int readers_in_reading_room = 0;   // Liczba czytelników aktualnie przebywających w czytelni
volatile int num_writter_in_reading_room = -1;   // Numer pisarza akutalnie przebywającego w czytelni
volatile bool first_writter_was_in_reading_room = false;    // Flaga informująca czy od momentu uruchomienia programu w czytelni pojawił sie pisarz
volatile int readers_entered_count = 0; // Licznik czytelników którzy weszli do czytelni

FIFO* readersQueue; //Kolejka FIFO oczekujących czytelników
FIFO* writersQueue; //Kolejka FIFO oczekujących pisarzy
Node *readers_list = NULL;  // Lista czytelników aktualnie przebywających w czytelni
Node *readers_entered = NULL;  // Lista czytelników aktualnie przebywających w czytelni

TicketLock ticket_lock_reader_in;  //TicketLock wejściowy czytelnika
pthread_mutex_t mutex;  // Mutex czytelnika
pthread_mutex_t writer_mutex;   // Mutex pisarza
sem_t writer_access;    // Semafor zezwalający pisarzowi na wejście

void *reading(void *arg);
void *writeing(void *arg);
void Initialize();
void Destroy();
void show_queue_and_reading_room();

int main(int argc, char* argv[]) {
    initArgs(argc, argv);   // Sprawdzenie i wczytanie podanych argumentów

    Initialize();   // Inicjalizacja mutexów semaforów i ticket locków

    srand(time(NULL));  // Ustawienie generatora liczb pseudolosowych

    // Tablice wątków pisarzy i czytelników
    pthread_t writers_threads[writers_count];
    pthread_t readers_threads[readers_count];

    readersQueue = createQueue();   // Utworzenie kolejki FIFO oczekujących czytelników
    writersQueue = createQueue();   // Utworzenie kolejki FIFO oczekujących pisarzy

    int i = 0;
    int iret1;
     // Pętla tworząca wątki czytelników
    for(i = 0; i < readers_count; i++){
        ReaderThreadData* reader_threadData = malloc(sizeof(ReaderThreadData)); //Alokacja pamięci na dane wątku przekazywane do funkcji
        if (reader_threadData == NULL) {    // Jeśli wystąpił błąd podczas alokacji wypisz komunikat i zakończ program
            fprintf(stderr, "Błąd alokacji pamięci reader_threadData\n");
            exit(EXIT_FAILURE);
        }
        reader_threadData->readersQueue = readersQueue; // Każdy wątek czytelnika modyfikuje tą samą kolejkę readersQueue
        reader_threadData->reader_num = malloc(sizeof(int));    // Alokacja pamięci na numer wątku danego czytelnika
        if (reader_threadData->reader_num == NULL) {    // Jeśli wystąpił błąd podczas alokacji wypisz komunikat i zakończ program
            fprintf(stderr, "Błąd alokacji pamięci reader_num\n");
            exit(EXIT_FAILURE);
        }
        *(reader_threadData->reader_num) = i;   // Numerem wątku jest numer iteracji pętli for w której został on utworzony
        // Utworzenie wątku wykonującego funckję reading wraz z parametrami przekazanymi jako struktura reader_threadData
        iret1 = pthread_create(&readers_threads[i], NULL, reading, (void*)reader_threadData);
        if(iret1) { // Jeśli wystąpił błąd podczas tworzenia wątku wypisz komunikat i zakończ program
            fprintf(stderr,"Błąd tworzenia %d wątku czytelnika. Kod błędu: %d\n", i, iret1);
            exit(EXIT_FAILURE);
        }
    }

    // Pętla tworząca wątki pisarzy
    for(i = 0; i < writers_count; i++){
        WriterThreadData* writer_threadData = malloc(sizeof(WriterThreadData)); // Alokacja pamięci na dane wątku przekazywane do funkcji
        if (writer_threadData == NULL) {    // Jeśli wystąpił błąd podczas alokacji wypisz komunikat i zakończ program
            fprintf(stderr, "Błąd alokacji pamięci writer_threadData\n");
            exit(EXIT_FAILURE);
        }
        writer_threadData->writersQueue = writersQueue; // Każdy wątek pisarza modyfikuje tą samą kolejkę writersQueue
        writer_threadData->writer_num = malloc(sizeof(int));    // Alokacja pamięci na numer wątku danego pisarza
        if (writer_threadData->writer_num == NULL) {    // Jeśli wystąpił błąd podczas alokacji wypisz komunikat i zakończ program
            fprintf(stderr, "Błąd alokacji pamięci writer_num\n");
            exit(EXIT_FAILURE);
        }
        *(writer_threadData->writer_num) = i;   // Numerem wątku jest numer iteracji pętli for w której został on utworzony
        //Utworzenie wątku wykonującego funckję writeing wraz z parametrami przekazanymi jako struktura writer_threadData
        iret1 = pthread_create(&writers_threads[i], NULL, writeing, (void*)writer_threadData);  
        if(iret1) { // Jeśli wystąpił błąd podczas tworzenia wątku wypisz komunikat i zakończ program
            fprintf(stderr,"Błąd tworzenia %d wątku pisarza. Kod błędu: %d\n", i, iret1);
            exit(EXIT_FAILURE);
        }
    }

    // Oczekiwanie na zakończenie wątków czytelników readers_threads
    for(i = 0; i < readers_count; i++){
        iret1 = pthread_join(readers_threads[i], NULL);
        if (!iret1) {   // Jeśli pthread_join wykonało się niepoprawnie wypisz komunikat i zakończ program
            fprintf(stderr, "Błąd dołączania %d wątku czytelnika. Kod błędu: %d\n", i, iret1);
            exit(EXIT_FAILURE);
        }
    }
    // Oczekiwanie na zakończenie wątków pisarzy writers_threads
    for(i = 0; i < writers_count; i++){
        iret1 = pthread_join(writers_threads[i], NULL);
        if (!iret1) {   // Jeśli pthread_join wykonało się niepoprawnie wypisz komunikat i zakończ program
            fprintf(stderr, "Błąd dołączania %d wątku pisarza. Kod błędu: %d\n", i, iret1);
            exit(EXIT_FAILURE);
        }
    }

    Destroy();  //Usuniecie zmiennych warunowych i ticket locków

    return 0;
}



// Funkcja wykonywana przez czytelników
void *reading(void *arg) {
    int iret1;
    ReaderThreadData* reader_threadData = (ReaderThreadData*)arg;   // Konwersja otrzymanego parametru na strukturę ReaderThreadData
    FIFO* queue = reader_threadData->readersQueue;  // Przypisanie kolejki czytelników do zmiennej queue (w celu skrócenia zapisu)
    enqueue(queue, *(reader_threadData->reader_num));   // Dodanie do kolejki wszystkich czytelników
    while (1) { 

/////////////////////////////////////// SEKCJA KRYTYCZNA - WEJSCIE CZYTELNIKA /////////////////////////////////////////////////////

        // Zablokowanie sekcji krytycznej
        iret1 = pthread_mutex_lock(&mutex); 
        if(iret1){
            fprintf(stderr, "Błąd zablokowania mutexu mutex (wejście czytelnika). Kod błędu: %d\n", iret1);
            exit(EXIT_FAILURE);
        }
            // Jeśli są pisarze w czytelni lub czytelnik był już w czytelni - ustaw się w kolejce
            if (writers_in_reading_room > 0 || element_exists(readers_entered, *(reader_threadData->reader_num))) {     
                enqueue(queue, *(reader_threadData->reader_num));   // Dodanie numeru wątku do kolejki FIFO

                // Jeśli kolejka jest pełna - wszyscy czytelnicy stoją w kolejce udziel dostępu do czytelni pisarzowi
                if(queueCapacity(queue) == readers_count){  
                    writer_enter=false;
                    iret1 = sem_post(&writer_access);   // Wyślij sygnał pisarzowi że może wejść do czytelni
                    if(iret1){
                        fprintf(stderr, "Błąd wykonania operacji post na semaforze writer_access. Kod błędu: %d\n", iret1);
                        exit(EXIT_FAILURE);
                    }
                }

                // Odblokowanie sekcji krytycznej tak aby kolejny wątek mógł zablokować mutex
                iret1 = pthread_mutex_unlock(&mutex);
                if(iret1){
                    fprintf(stderr, "Błąd odblokowania mutexu mutex (wejście czytelnika). Kod błędu: %d\n", iret1);
                    exit(EXIT_FAILURE);
                }

                // Zamknięcie ticket locka - zakolejkowanie wątku
                iret1 = lock(&ticket_lock_reader_in);   
                if(iret1){
                    fprintf(stderr, "Błąd zablokowania ticket_lock_reader_in. Kod błędu: %d\n", iret1);
                    exit(EXIT_FAILURE);
                }
                
                // Ponowne zablokowanie sekcji krytycznej 
                iret1 = pthread_mutex_lock(&mutex);
                if(iret1){
                    fprintf(stderr, "Błąd zablokowania mutexu mutex (wejście czytelnika). Kod błędu: %d\n", iret1);
                    exit(EXIT_FAILURE);
                }
                readers_entered_count++;    // Zwiększenie licznika czytelników, którzy weszli

            }
            // UWAGA
            // Przy zakomentowaniu wszystkich sleep, aby program nadal działał należy zakomentować także ten warunek
            // Co prawda wtedy do czytelni na zmiane wchodzi jeden pisarz i jeden czytelnik (w odpowiedniej kolejności)
            // Ale unikane jest wystąpienie zjawiska blokady. 
            if(first_writter_was_in_reading_room && readers_entered_count < readers_count){
                    iret1 = unlock(&ticket_lock_reader_in);
                    if(iret1){
                        fprintf(stderr, "Błąd odblokowania ticket_lock_reader_in. Kod błędu: %d\n", iret1);
                        exit(EXIT_FAILURE);
                    }
            }
                show_queue_and_reading_room();  // Wypisane informacji na ekranie o stanie kolejek i osób w czytelni
                // Jeśli nie ma pisarzy w czytelni i wątek (czytelnik) o danym numerze nie był jeszcze w czytelni
                readers_in_reading_room++;  // Czytelnik wchodzi do czytelni (zwiększenie licznika czytelników w czytelni o 1)
                dequeue(queue); // Usunięcie pierwszego czytelnika z kolejki czytelników 
                insert_element(&readers_list, *(reader_threadData->reader_num));    // Dodanie jego numeru do listy osób przebywających w czytelni  
                show_queue_and_reading_room();  // Wypisane informacji na ekranie o stanie kolejek i osób w czytelni
        
        // Odblokowanie sekcji krytycznej
        iret1 = pthread_mutex_unlock(&mutex);   
        if(iret1){
            fprintf(stderr, "Błąd oblokowania mutexu mutex (wejście czytelnika). Kod błędu: %d\n", iret1);
            exit(EXIT_FAILURE);
        }

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

        sleep(random_sleep_time()); // Czytelnik znajduje się w czytelni i czyta przez losowy czas

/////////////////////////////////////// SEKCJA KRYTYCZNA - WYJSCIE CZYTELNIKA /////////////////////////////////////////////////////
        
        // Zablokowanie sekcji krytycznej
        iret1 = pthread_mutex_lock(&mutex); 
        if(iret1){
            fprintf(stderr, "Błąd zablokowania mutexu mutex (wyjście czytelnika). Kod błędu: %d\n", iret1);
            exit(EXIT_FAILURE);
        }
            readers_in_reading_room--;  // Czytelnik opuszcza czytelnie (zmniejszenie licznika czytelników w czytelni o 1)
            remove_element(&readers_list, *(reader_threadData->reader_num));    // Usunięcie numeru wątku czytelnika z listy osób przebywających w czytelni
            insert_element(&readers_entered,*(reader_threadData->reader_num));  // Dodanie numeru wątku czytelnika do listy osób które odwiedziły już czytelnie

        // Oblokowanie sekcji krytycznej 
        iret1 = pthread_mutex_unlock(&mutex); 
        if(iret1){
            fprintf(stderr, "Błąd oblokowania mutexu mutex (wyjście czytelnika). Kod błędu: %d\n", iret1);
            exit(EXIT_FAILURE);
        }

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

        sleep(random_sleep_time()); // Czytelnik czeka pewien losowy czas przed kolejną próbą dostania się do czytelni
    }
}


// Funkcja wykonywana przez pisarzy
void *writeing(void *arg) {
    int iret1;
    WriterThreadData* writer_threadData = (WriterThreadData*)arg;   // Konwersja otrzymanego parametru na strukturę WriterThreadData
    FIFO* queue = writer_threadData->writersQueue;  // Przypisanie kolejki pisarzy do zmiennej queue (w celu skrócenia zapisu)
    while (1) {
        enqueue(queue, *(writer_threadData->writer_num));   // Dodanie numeru wątku pisarza do kolejki

/////////////////////////////////////// SEKCJA KRYTYCZNA - WEJSCIE/WYJŚCIE PISARZA /////////////////////////////////////////////////////

        // Zablokowanie sekcji krytycznej
        iret1 = pthread_mutex_lock(&writer_mutex);
        if(iret1){
            fprintf(stderr, "Błąd zablokowania mutexu writer_mutex (wejście pisarza). Kod błędu: %d\n", iret1);
            exit(EXIT_FAILURE);
        }
        
            // Dopóki czytelnicy są w czytelni lub jeden z pisarzy jest w czytelni oczekuj na semaforze writer_access
            // W czytelni może przebywać tylko jeden pisarz dlatego konieczne jest oczekiwanie pozostałych na wejście 
            while (writer_enter||readers_in_reading_room > 0 || writers_in_reading_room > 0) {
                iret1 = sem_wait(&writer_access);   // Blokada dostępu pisarzom
                if(iret1){
                    fprintf(stderr, "Błąd wykonania operacji wait na semaforze writer_access. Kod błędu: %d\n", iret1);
                    exit(EXIT_FAILURE);
                }
            }
            show_queue_and_reading_room();  // Wypisanie stanu kolejek i liczby osób w czytelni
            writers_in_reading_room++;  // Pisarz wchodzi do czytelni (zwiększenie licznika pisarzy w czytelni o 1)
            first_writter_was_in_reading_room = true;   // Pierwszy z pisarzy wszedł do czytelni
            readers_entered_count = 0;  // Wyzerowanie licznika czytelników którzy weszli do czytelni
            dequeue(queue); // Pierwszy z pisarzy w kolejce jest usuwany z kolejki (wszedł on do czytelni)
            num_writter_in_reading_room = *(writer_threadData->writer_num); // Wyznaczenie numeru pisarza który właśnie wszedł do czytelni 
            show_queue_and_reading_room();  //  Wypisanie informacji o stanie kolejek i liczbie osób w czytelni

        

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

        sleep(random_sleep_time()); // Pisarz przebywa w czytelni przez pewnien losowy czas


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

            writers_in_reading_room--;  // Pisarz wychodzi z czytelni (zmiejszenie licznika pisarzy w czytelni o 1)
            num_writter_in_reading_room = -1;   // Aktualnie w czytelni nie przebywa żaden pisarz 
            writer_enter = true;    // Pisarz wyszedł z czytelni
            clear_list(&readers_entered);   // Wyczyszczenie listy czytelników którzy wyszli z czytelni
            
            // Oblokowanie Ticket Locka czytelników - wpuszczenie jednego czytelnika do czytelni (pierwszego z kolejki)
            iret1 = unlock(&ticket_lock_reader_in);
            if(iret1){
                fprintf(stderr, "Błąd odblokowania ticket_lock_reader_in. Kod błędu: %d\n", iret1);
                exit(EXIT_FAILURE);
            }
        
        // Odblokowanie sekcji krytycznej
        iret1 = pthread_mutex_unlock(&writer_mutex);   
        if(iret1){
            fprintf(stderr, "Błąd odblokowania mutexu writer_mutex (wyjście pisarza). Kod błędu: %d\n", iret1);
            exit(EXIT_FAILURE);
        }
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

        sleep(random_sleep_time()); // Pisarz oczekuje losowy czas przed kolejną próbą dostania się do czytelni (usatwienia się w kolejce)
    }
}

// Funckja Initialize inicjalizuje wszytskie potrzebne do działania programu komponenty: mutexy, semafor, ticketLock
// Sprawdza także czy każda z inicjalizacji przebiegła pomyślnie
// Jeśi nie to wypisywana jest informacja o błędzie na STDERR i program jest kończony
void Initialize(){
    int iret1;
    iret1 = sem_init(&writer_access, 0, 1);
    if(iret1){
        fprintf(stderr, "Błąd inicjacji semafora writter_acces. Kod błędu: %d\n", iret1);
        exit(EXIT_FAILURE);
    }
    iret1 = pthread_mutex_init(&mutex, NULL);
    if(iret1){
        fprintf(stderr, "Blad tworzenia mutexu mutex. Kod błędu: %d", iret1);
        exit(EXIT_FAILURE);
    }
    iret1 = pthread_mutex_init(&writer_mutex, NULL);
    if(iret1){
        fprintf(stderr, "Blad tworzenia mutexu writer_mutex. Kod błędu: %d", iret1);
        exit(EXIT_FAILURE);
    }
    iret1 = init_ticket_lock(&ticket_lock_reader_in);
    if(iret1){
        fprintf(stderr, "Błąd inicjacji ticket lock ticket_lock_reader_in. Kod błędu: %d\n", iret1);
        exit(EXIT_FAILURE);
    }
}

// Funckja Destroy niszczy wszytskie komponenty po zakończeniu działania programu: mutexy, semafor, ticketLock
// Sprawdza także czy każde zniszczenie przebiegło pomyślnie
// Jeśi nie to wypisywana jest informacja o błędzie na STDERR i program jest kończony
void Destroy(){
    int iret1;
    iret1 = pthread_mutex_destroy(&mutex);
    if(iret1){
        fprintf(stderr, "Błąd niszczenia muetxu mutex. Kod błędu: %d\n", iret1);
        exit(EXIT_FAILURE);
    }
    iret1 = pthread_mutex_destroy(&writer_mutex);
    if(iret1){
        fprintf(stderr, "Błąd niszczenia muetxu writer_mutex. Kod błędu: %d\n", iret1);
        exit(EXIT_FAILURE);
    }
    iret1 = sem_destroy(&writer_access);
    if (iret1){
        fprintf(stderr, "Błąd niszczenia semafora writter_acces. Kod błędu: %d\n", iret1);
        exit(EXIT_FAILURE);
    }
    iret1 = destroy_ticket_lock(&ticket_lock_reader_in);
    if(iret1){
        fprintf(stderr, "Błąd niszczenia ticket lock ticket_lock_reader_in. Kod błędu: %d\n", iret1);
        exit(EXIT_FAILURE);
    }
}

// Funkcja służąca do wypisywania informacji o stanie kolejek i osobach w czytelni
void show_queue_and_reading_room(){

    // W podstawowym przypadku wypisywany jest rozmiar poszczególnych kolejek oraz ilość poszczególnych osób w czytelni
    printf("Kczytelnikow: %d Kpisarzy: %d [C: %d P: %d]\n", queueCapacity(readersQueue), queueCapacity(writersQueue),  readers_in_reading_room, writers_in_reading_room);

    // Warunek ten uwzględnia podanie jako parametr wartości "-info"
    // W takim przypadku dodatkowo wypisywana jest pełna zawartość poszczególnych kolejek oraz numery wątków przebywających w czytelni
    if(flag_info){
        printf("\nKolejka czytelników: ");
        displayQueue(readersQueue); // Wyświetl kolejkę czytelników
        printf("\nKolejka pisarzy: ");
        displayQueue(writersQueue); // Wyśiwetl kolejkę pisarzy
        printf("\nPisarz przebywający w czytelni: ");
        // Jeśli żaden z pisarzy nie znajduje się w czytelni wypisz stosowny komunikat
        // Numery pisarzy zaczynają się od 0 dlatego zastosowano wartość -1
        if(num_writter_in_reading_room == -1){
            printf("Brak\n");
        }
        // Jeśli którykolwiek z pisarzy znajduje się w czytelni wypisz jego numer
        else{
            printf("%d\n", num_writter_in_reading_room);
        }
        printf("\nCzytelnicy przebywający w czytelni: ");
        print_list(readers_list);   // Wypisz listę czytelników przebywających w czytelni
        printf("\nCzytelnicy którzy wyszli z czytelni: ");
        print_list(readers_entered);    // Wypisz listę czytelników, którzy wyszli z czytelni
        printf("------------------------------------------\n");
    }
}