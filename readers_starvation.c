#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <pthread.h>
#include <time.h>
#include <unistd.h>
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
volatile int writers_count = 0; // Liczba pisarzy podana jako parametr programu
volatile bool flag_info = false;    // Flaga wskazująca na to czy został podany parametr "-info"
volatile int writers_in_reading_room = 0;   // Liczba pisarzy aktualnie przebywających w czytelni
volatile int readers_in_reading_room = 0;   // Liczba czytelników aktualnie przebywających w czytelni
volatile int num_writter_in_reading_room = -1;   // Numer pisarza akutalnie przebywającego w czytelni
volatile int writers_in_queue = 0;  // Liczba pisarzy oczekujących w kolejce do czytelni

FIFO* readersQueue; //Kolejka FIFO oczekujących czytelników
FIFO* writersQueue; //Kolejka FIFO oczekujących pisarzy
Node *readers_list = NULL;  // Lista czytelników aktualnie przebywających w czytelni

TicketLock ticket_lock_reader_in;  //TicketLock wejściowy czytelnika
TicketLock ticket_lock_writer_in;  //TicketLock wejściowy pisarza
pthread_cond_t writerCond;  // Zmienna waurnkowa pisarza
pthread_cond_t readerCond;  // Zmienna warunkowa czytelnika
pthread_mutex_t mutex;  // Mutex pisarza i czytelnika

void *reading(void *arg);
void *writeing(void *arg);
void Initialize();
void Destroy();
void show_queue_and_reading_room();

int main(int argc, char* argv[]){

    initArgs(argc, argv);   // Sprawdzenie i wczytanie podanych argumentów

    Initialize();   // Inicjalizacja mutexów zmiennych warunkowych i ticket locków

    srand(time(NULL));  // Ustawienie generatora liczb pseudolosowych

    // Tablice wątków pisarzy i czytelników
    pthread_t writers_threads[writers_count];
    pthread_t readers_threads[readers_count];

    readersQueue = createQueue();   // Utworzenie kolejki FIFO oczekujących czytelników
    writersQueue = createQueue();   // Utworzenie kolejki FIFO oczekujących pisarzy

    int i = 0;
    int iret1;

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

    // Oczekiwanie na zakończenie wątków pisarzy writers_threads
    for(i = 0; i < writers_count; i++){
        iret1 = pthread_join(writers_threads[i], NULL);
        if (!iret1) {   // Jeśli pthread_join wykonało się niepoprawnie wypisz komunikat i zakończ program
            fprintf(stderr, "Błąd dołączania %d wątku pisarza. Kod błędu: %d\n", i, iret1);
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
    
    Destroy();  //Usuniecie mutexow, zmiennych warunkowych i ticket locków
    return 0;
}


// Funkcja wykonywana przez czytelników
void *reading(void *arg) {
    int iret1;
    ReaderThreadData* reader_threadData = (ReaderThreadData*)arg;   // Konwersja otrzymanego parametru na strukturę ReaderThreadData
    FIFO* queue = reader_threadData->readersQueue;  // Przypisanie kolejki czytelników do zmiennej queue (w celu skrócenia zapisu)
    enqueue(queue, *(reader_threadData->reader_num));   // Dodanie do kolejki oczekujących czytelników wszystkich wątków które pierwszy raz weszły do funckji
    show_queue_and_reading_room();  // Wypisane informacji na ekranie o stanie kolejek i osób w czytelni
    while (1) {

/////////////////////////////////////// SEKCJA KRYTYCZNA - WEJSCIE CZYTELNIKA /////////////////////////////////////////////////////

        // Dodanie czytelnika do kolejki FIFO (przydzielenie ticketu)
        iret1 = lock(&ticket_lock_reader_in);
        if(iret1){
            fprintf(stderr, "Błąd zablokowania ticket_lock_reader_in. Kod błędu: %d\n", iret1);
            exit(EXIT_FAILURE);
        }
        // Blokada sekcji krytycznej - mutex potrzebny do pthread_cond_wait
        iret1 = pthread_mutex_lock(&mutex);
        if(iret1){
            fprintf(stderr, "Błąd zablokowania mutexu mutex (wejście czytelnika). Kod błędu: %d\n", iret1);
            exit(EXIT_FAILURE);
        }

            // Dopóki są pisarze w czytelni lub stoją w kolejce zatrzymaj wątek na zmiennej warunkowej
            // Tym samym odblokuj sekcję krtyczną zwalniając mutex mutex
            while (writers_in_reading_room > 0 || writers_in_queue > 0) { 
                iret1 = pthread_cond_wait(&readerCond, &mutex);
                if(iret1){
                    fprintf(stderr, "Błąd wykonania operacji wait zmiennej warunkowej readerCond. Kod błędu: %d\n", iret1);
                    exit(EXIT_FAILURE);
                }
            }

            // Jeśli nie ma pisarzy w czytelni i żaden z pisarzy nie stoi w kolejce
            show_queue_and_reading_room();  // Wypisane informacji na ekranie o stanie kolejek i osób w czytelni
            readers_in_reading_room++;  // Czytelnik wchodzi do czytelni (zwiększenie licznika czytelników w czytelni o 1)
            dequeue(queue); // Usunięcie pierwszego czytelnika z kolejki czytelników 
            insert_element(&readers_list, *(reader_threadData->reader_num));    // Dodanie jego numeru do listy osób przebywających w czytelni   
            show_queue_and_reading_room();  // Wypisane informacji na ekranie o stanie kolejek i osób w czytelni

         // Odblokowanie sekcji krytycznej
        iret1 = pthread_mutex_unlock(&mutex);  
        if(iret1){
            fprintf(stderr, "Błąd odblokowania mutexu mutex (wejście czytelnika). Kod błędu: %d\n", iret1);
            exit(EXIT_FAILURE);
        }

        // Zwolnienie ticketLocka - zwiększenie numeru ticketu i obudzenie wszystkich wątków do sprawdzenia swojego ticketu
        iret1 = unlock(&ticket_lock_reader_in);
        if(iret1){
            fprintf(stderr, "Błąd odblokowania ticket_lock_reader_in. Kod błędu: %d\n", iret1);
            exit(EXIT_FAILURE);
        }

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

        sleep(random_sleep_time()); // Czytelnik znajduje się w czytelni i czyta przez losowy czas


/////////////////////////////////////// SEKCJA KRYTYCZNA - WYJSCIE CZYTELNIKA /////////////////////////////////////////////////////

        // Blokada sekcji krytycznej
        iret1 = pthread_mutex_lock(&mutex);
        if(iret1){
            fprintf(stderr, "Błąd zablokowania mutexu mutex (wyjście czytelnika). Kod błędu: %d\n", iret1);
            exit(EXIT_FAILURE);
        }

            readers_in_reading_room--;  // Czytelnik opuszcza czytelnie (zmniejszenie licznika czytelników w czytelni o 1)  
            remove_element(&readers_list, *(reader_threadData->reader_num));    // Usunięcie numeru wątku czytelnika z listy osób przebywających w czytelni

            // Jeśli nie ma już żadnych czytelników w czytelni - zezwól pisarzom na wejście do czytelni
            // Wykonywana jest operacja signal na zmiennej warunkowej writerCond gdyż tylko jeden pisarz może wejść do czytelni
            if (readers_in_reading_room == 0) {
                iret1 = pthread_cond_signal(&writerCond);
                if(iret1){
                    fprintf(stderr, "Błąd wykonania operacji signal na zmiennej warunkowej writerCond. Kod błędu: %d\n", iret1);
                    exit(EXIT_FAILURE);
                }
            }

        // Odblokowanie sekcji krytycznej
        iret1 = pthread_mutex_unlock(&mutex);
        if(iret1){
            fprintf(stderr, "Błąd odblokowania mutexu mutex (wyjście czytelnika). Kod błędu: %d\n", iret1);
            exit(EXIT_FAILURE);
        }

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

        sleep(random_sleep_time()); // Czytelnik czeka pewien losowy czas przed kolejną próbą dostania się do czytelni
        enqueue(queue, *(reader_threadData->reader_num)); // Jeśli czytelnik ponownie decyduje się na wejście jest dodawany do kolejki 
    }
}


// Funkcja wykonywana przez pisarzy
void *writeing(void *arg) {
    int iret1;
    WriterThreadData* writer_threadData = (WriterThreadData*)arg;   // Konwersja otrzymanego parametru na strukturę WriterThreadData
    FIFO* queue = writer_threadData->writersQueue;  // Przypisanie kolejki pisarzy do zmiennej queue (w celu skrócenia zapisu)
    enqueue(queue, *(writer_threadData->writer_num));   // Dodanie do kolejki wszystkich pisarzy, którzy pierwszy raz wchodzą do funckji
    show_queue_and_reading_room();  // Wypisanie stanu kolejek i liczby osób w czytelni
    while (1) {

/////////////////////////////////////// SEKCJA KRYTYCZNA - WEJSCIE PISARZA /////////////////////////////////////////////////////
        
        // Dodanie czytelnika do kolejki FIFO (przydzielenie ticketu)
        iret1 = lock(&ticket_lock_writer_in);   // Zamknięcie ticket locka (przydzielenie ticketu wątkowi)
        if(iret1){
            fprintf(stderr, "Błąd zablokowania ticket_lock_writer_in. Kod błędu: %d\n", iret1);
            exit(EXIT_FAILURE);
        }
        // Blokada sekcji krytycznej - mutex potrzebny do pthread_cond_wait
        iret1 = pthread_mutex_lock(&mutex);
        if(iret1){
            fprintf(stderr, "Błąd zablokowania mutexu mutex (wejście pisarza). Kod błędu: %d\n", iret1);
            exit(EXIT_FAILURE);
        }

            // Dopóki czytelnicy są w czytelni lub jeden z pisarzy jest w czytelni oczekuj na zmiennej warunkowej writerCond
            // W czytelni może przebywać tylko jeden pisarz dlatego konieczne jest oczekiwanie pozostałych na wejście
            while (readers_in_reading_room > 0 || writers_in_reading_room > 0) {
                iret1 = pthread_cond_wait(&writerCond, &mutex);
                if (iret1){
                    fprintf(stderr, "Błąd wykonania operacji wait na zmiennej warunkowej writerCond. Kod błędu: %d\n", iret1);
                    exit(EXIT_FAILURE);
                }
            }
            show_queue_and_reading_room();  // Wypisanie stanu kolejek i liczby osób w czytelni
            writers_in_reading_room++;  // Pisarz wchodzi do czytelni (zwiększenie licznika pisarzy w czytelni o 1)
            dequeue(queue); // Pierwszy z pisarzy w kolejce jest usuwany z kolejki (wszedł on do czytelni)
            num_writter_in_reading_room = *(writer_threadData->writer_num); // Wyznaczenie numeru pisarza który właśnie wszedł do czytelni 
            show_queue_and_reading_room();  //  Wypisanie informacji o stanie kolejek i liczbie osób w czytelni
        
        // Odblokwanie sekcji krytycznej
        iret1 = pthread_mutex_unlock(&mutex);
        if(iret1){
            fprintf(stderr, "Błąd odblokowania mutexu mutex (wejście pisarza). Kod błędu: %d\n", iret1);
            exit(EXIT_FAILURE);
        }
        // Zwolnienie ticketLocka - zwiększenie numeru ticketu i obudzenie wszystkich wątków do sprawdzenia swojego ticketu
        iret1 = unlock(&ticket_lock_writer_in);
        if(iret1){
            fprintf(stderr, "Błąd odblokowania ticket_lock_writer_in. Kod błędu: %d\n", iret1);
            exit(EXIT_FAILURE);
        }

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

        sleep(random_sleep_time()); // Pisarz przebywa w czytelni przez pewnien losowy czas


/////////////////////////////////////// SEKCJA KRYTYCZNA - WYJSCIE PISARZA /////////////////////////////////////////////////////

        iret1 = pthread_mutex_lock(&mutex);
        if(iret1){
            fprintf(stderr, "Błąd zablokowania mutexu mutex (wyjście pisarza). Kod błędu: %d\n", iret1);
            exit(EXIT_FAILURE);
        }

            writers_in_reading_room--;  // Pisarz wychodzi z czytelni (zmiejszenie licznika pisarzy w czytelni o 1)
            writers_in_queue = queueCapacity(queue);    // Pobranie ilości elementów kolejki pisarzy
            num_writter_in_reading_room = -1;   // Brak pisarzy w czytelni

            // Jeśli dany pisarz wychodzi z czytelni, a do tej pory nie ma pisarzy w kolejce przed czytelnią
            // Obudź wszytekie wątki czytelników zatrzymane na zmiennej warunkowej readerCond umożliwiając im dostęp do czytelni
            if (writers_in_queue == 0) {
                iret1 = pthread_cond_broadcast(&readerCond);
                if(iret1){
                    fprintf(stderr, "Błąd wykonania operacji broadcast na zmiennej warunkowej readerCond. Kod błędu: %d\n", iret1);
                    exit(EXIT_FAILURE);
                }
            }

            // Jeśli dany pisarz wychodzi z czytelni, a w kolejece do czytelni czekają kolejni pisarze
            // Daj sygnał innemu pisarzowi, że może wejśc do czytelni
            // Wykonywana jest operacja signal gdyż budzi ona tylko jeden wątek, a tylko jeden pisarz może wejść do czytelni
            else{
                iret1 = pthread_cond_signal(&writerCond);
                if(iret1){
                    fprintf(stderr, "Błąd wykonania operacji signal na zmiennej warunkowej writerCond. Kod błędu: %d\n", iret1);
                    exit(EXIT_FAILURE);
                }
            }
        // Odblokowanie sekcji krytycznej
        iret1 = pthread_mutex_unlock(&mutex);
        if(iret1){
            fprintf(stderr, "Błąd odblokowania mutexu mutex (wyjście pisarza). Kod błędu: %d\n", iret1);
            exit(EXIT_FAILURE);
        }

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

        sleep(random_sleep_time()); // Pisarz oczekuje losowy czas przed kolejną próbą dostania się do czytelni (usatwienia się w kolejce)
        // Po odczekaniu pisarz próbuje dostać się do czytelni i staje w kolejce a jego numer dodawany jest do kolejki FIFO
        enqueue(queue, *(writer_threadData->writer_num));
    }
}


// Funckja Initialize inicjalizuje wszytskie potrzebne do działania programu komponenty: mutexy, zmienne warunkowe, ticketLocki
// Sprawdza także czy każda z inicjalizacji przebiegła pomyślnie
// Jeśi nie to wypisywana jest informacja o błędzie na STDERR i program jest kończony
void Initialize(){
    int iret1;
    iret1 = init_ticket_lock2(&ticket_lock_reader_in);
    if(iret1){
        fprintf(stderr, "Błąd inicjacji ticket lock ticket_lock_reader_in. Kod błędu: %d\n", iret1);
        exit(EXIT_FAILURE);
    }
    iret1 = init_ticket_lock2(&ticket_lock_writer_in);
    if(iret1){
        fprintf(stderr, "Błąd inicjacji ticket lock ticket_lock_writer_in. Kod błędu: %d\n", iret1);
        exit(EXIT_FAILURE);
    }  
    iret1 = pthread_cond_init(&writerCond, NULL);
    if(iret1){
        fprintf(stderr, "Blad tworzenia zmiennej warunkowej writerCond. Kod błędu: %d", iret1);
        exit(EXIT_FAILURE);
    }
    iret1 = pthread_cond_init(&readerCond, NULL);
    if(iret1){
        fprintf(stderr, "Blad tworzenia zmiennej warunkowej readerCond. Kod błędu: %d", iret1);
        exit(EXIT_FAILURE);
    }
    iret1 = pthread_mutex_init(&mutex, NULL);
    if(iret1){
        fprintf(stderr, "Blad tworzenia mutexu mutex. Kod błędu: %d", iret1);
        exit(EXIT_FAILURE);
    }
}

// Funckja Destroy niszczy wszytskie komponenty po zakończeniu działania programu: mutexy, zmienne warunkowe, ticketLocki
// Sprawdza także czy każde zniszczenie przebiegło pomyślnie
// Jeśi nie to wypisywana jest informacja o błędzie na STDERR i program jest kończony
void Destroy(){
    int iret1;
    iret1 = pthread_cond_destroy(&writerCond);
    if(iret1){
        fprintf(stderr, "Błąd niszczenia zmiennej warunkowej writerCond. Kod błędu: %d\n", iret1);
        exit(EXIT_FAILURE);
    }
    iret1 = pthread_cond_destroy(&readerCond);
    if(iret1){
        fprintf(stderr, "Błąd niszczenia zmiennej warunkowej readerCond. Kod błędu: %d\n", iret1);
        exit(EXIT_FAILURE);
    }
    iret1 = pthread_mutex_destroy(&mutex);
    if(iret1){
        fprintf(stderr, "Błąd niszczenia muetxu mutex. Kod błędu: %d\n", iret1);
        exit(EXIT_FAILURE);
    }
    iret1 = destroy_ticket_lock(&ticket_lock_reader_in);
    if(iret1){
        fprintf(stderr, "Błąd niszczenia ticket lock ticket_lock_reader_in. Kod błędu: %d\n", iret1);
        exit(EXIT_FAILURE);
    }
    iret1 = destroy_ticket_lock(&ticket_lock_writer_in);
    if(iret1){
        fprintf(stderr, "Błąd niszczenia ticket lock ticket_lock_writer_in. Kod błędu: %d\n", iret1);
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
        printf("------------------------------------------\n");
    }
}