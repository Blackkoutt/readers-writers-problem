#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <pthread.h>
#include <time.h>
#include <unistd.h>
#include <semaphore.h>
#include "ticket_lock.h"
#include "fifo.h"
#include "list.h"
#include "validate.h"

// Struktura parametrów przekazywanych do funkcji wykonywanej przez pisarza
typedef struct {
    FIFO* writersQueue; // Kolejka pisarzy oczekujących na wejście do czytelni
    int* writer_num;    // Numer wątku danego pisarza
} WriterThreadData;

// Struktura parametrów przekazywanych do funkcji wykonywanej przez czytelnika
typedef struct {
    FIFO* readersQueue; // Kolejka czytelników oczekujących na wejście do czytelni
    int* reader_num;    // Numer wątku danego czytelnika
} ReaderThreadData;

volatile int readers_count = 0; // Liczba czytelników podana jako parametr programu
volatile int writers_count = 0; // Liczba pisarzy podana jako parametr programu
volatile bool flag_info = false;    // Flaga wskazująca na to czy został podany parametr "-info"
volatile int writers_in_reading_room = 0;   // Liczba pisarzy aktualnie przebywających w czytelni
volatile int readers_in_reading_room = 0;   // Liczba czytelników aktualnie przebywających w czytelni
volatile int num_writter_in_reading_room = -1;   // Numer pisarza akutalnie przebywającego w czytelni

FIFO* readersQueue; //Kolejka FIFO oczekujących czytelników
FIFO* writersQueue; //Kolejka FIFO oczekujących pisarzy
Node *readers_list = NULL;  // Lista czytelników aktualnie przebywających w czytelni

TicketLock ticket_lock_writer;  // TicketLock pisarza
pthread_mutex_t reader_in_out;  //  Mutex tworzący sekcję krytyczną dla wejścia oraz wyjścia czytelnika z czytelni
sem_t writter_acces;    // Semafor sterujący dostępem pisarzy do czytelni

void *reading(void *arg);
void *writeing(void *arg);
void Initialize();
void Destroy();
void show_queue_and_reading_room();

int main(int argc, char* argv[]){
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
    
    Destroy();  //Usuniecie mutexow, semaforów i ticket locków
    return 0;
}



// Funkcja wykonywana przez czytelników
void *reading(void *arg){
    int iret1;
    ReaderThreadData* reader_threadData = (ReaderThreadData*)arg;   // Konwersja otrzymanego parametru na strukturę ReaderThreadData
    FIFO* queue = reader_threadData->readersQueue;  // Przypisanie kolejki czytelników do zmiennej queue (w celu skrócenia zapisu)
    // Dodanie do kolejki oczekujących czytelników wszystkich wątków które pierwszy raz weszły do funckji
    enqueue(queue, *(reader_threadData->reader_num));   
    show_queue_and_reading_room();  // Wypisane informacji na ekranie o stanie kolejek i osób w czytelni
    while(1){
        
/////////////////////////////////////// SEKCJA KRYTYCZNA - WEJSCIE CZYTELNIKA /////////////////////////////////////////////////////

        // Blokada sekcji krytycznej - dostęp do współdzielonej zmiennej 
        iret1 = pthread_mutex_lock(&reader_in_out); 
        if(iret1){
            fprintf(stderr, "Błąd zamkniecia mutexu reader_in_out (wejście czytelnika). Kod błędu: %d\n", iret1);
            exit(EXIT_FAILURE);
        }

            show_queue_and_reading_room(); // Wypisanie informacji o aktualnym stanie kolejek i osób w czytelni
            
            // Jeśli liczba czytelników w czytelni jest równa 0 i aktualnie będzie wchodzić do niej czytelnik
            // To pisarzom blokowany jest dostęp do czytelni poprzez semafor writter_acces
            if(readers_in_reading_room == 0){
                iret1 = sem_wait(&writter_acces);
                if(iret1){
                    fprintf(stderr, "Błąd wywolania operacji wait na semaforze writter_acces. Kod błędu: %d\n", iret1);
                    exit(EXIT_FAILURE);
                }
            }
            readers_in_reading_room++;  // Czytelnik wszedł do czytelni (zwiększenie licznika czytelników przebywających w czytelni)
            dequeue(queue); // Zdjęcie z kolejki FIFO pierwszego czytelnika
            insert_element(&readers_list, *(reader_threadData->reader_num));    // Dodanie tego czytelnika do listy czytelników w czytelni
            show_queue_and_reading_room();  //Wyświetlenie informacji o stanie kolejek i osob w czytelni
            
        // Odblokowanie sekcji krytycznej (czytelnik wszedł do czytelni, teraz będzie czytać)
        iret1 = pthread_mutex_unlock(&reader_in_out);
        if(iret1){
            fprintf(stderr, "Błąd otwarcia mutexu reader_in_out (wejście czytelnika). Kod błędu: %d\n", iret1);
            exit(EXIT_FAILURE);
        }

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


        sleep(random_sleep_time()); // Czytelnik czyta przez pewien losowy czas


/////////////////////////////////////// SEKCJA KRYTYCZNA - WYJSCIE CZYTELNIKA /////////////////////////////////////////////////////

        // Zablokowanie sekcji krytycznej - dostęp do współdzielonego zasobu
        iret1 = pthread_mutex_lock(&reader_in_out); 
        if(iret1){
            fprintf(stderr, "Błąd zamknięcia mutexu reader_in_out (wyjście czytelnika). Kod błędu: %d\n", iret1);
            exit(EXIT_FAILURE);
        }

            readers_in_reading_room--;  // Czytelnik wychodzi z czytelni  
            remove_element(&readers_list, *(reader_threadData->reader_num));    // Usunięcie danego czytelnika z listy czytelników przebywających w czytelni

            // Jeśli wyszli wszyscy czytelnicy (czytelnia jest pusta) dawana jest możliwość wejścia pisarzom poprzez 
            // wykonanie operacji post na semaforze writter_acces  
            if(readers_in_reading_room == 0){         
                iret1 = sem_post(&writter_acces);   // Wysłanie syganłu pisarzom, że czytelnia jest wolna
                if(iret1){
                    fprintf(stderr, "Błąd wywolania operacji post na semaforze writter_acces. Kod błędu: %d\n", iret1);
                    exit(EXIT_FAILURE);
                }
            }

        // Odblokowanie sekcji krytycznej
        iret1 = pthread_mutex_unlock(&reader_in_out);   
        if(iret1){
            fprintf(stderr, "Błąd otwarcia mutexu reader_in_out. Kod błędu: %d\n", iret1);
            exit(EXIT_FAILURE);
        }

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

        // Czytelnik czeka pewnien losowy czas po czym ponownie próbuje dostac się do czytelni ustawiając się w kolejce
        sleep(random_sleep_time()); 
        // Dodanie czytelnika do kolejki czytelników oczekujących na wejście
        enqueue(queue, *(reader_threadData->reader_num));   
    }
}




// Funkcja wykonywana przez pisarzy
void *writeing(void *arg){
    int iret1;
    WriterThreadData* writer_threadData = (WriterThreadData*)arg;   // Konwersja otrzymanego parametru na strukturę WriterThreadData
    FIFO* queue = writer_threadData->writersQueue;  // Przypisanie kolejki pisarzy do zmiennej queue (w celu skrócenia zapisu)
    enqueue(queue, *(writer_threadData->writer_num));   // Dodanie do kolejki wszystkich pisarzy, którzy pierwszy raz wchodzą do funckji
    show_queue_and_reading_room();  // Wypisanie stanu kolejek i liczby osób w czytelni
    while(1){ 

        // Dodanie pisarza do kolejki FIFO (przydzielenie ticketu) - wymagane przy zakomentowanych sleep 
        // W ten sposób kolejka pisarzy jest tworzona poprawnie (brak kilku tych samych pisarzy w kolejce)
        iret1 = lock(&ticket_lock_writer);
        if(iret1){
            fprintf(stderr, "Błąd zablokowania ticket_lock_writer. Kod błędu: %d\n", iret1);
            exit(EXIT_FAILURE);
        }

/////////////////////////////////////// SEKCJA KRYTYCZNA - WEJSCIE I WYJSCIE PISARZA /////////////////////////////////////////////////////  

        // Zablokowanie sekcji krytycznej poprzez semafor writter_acces
        // Pisarz oczekuje na możliwość wejścia do czytelni (możliwość wejścia sygnalizuje ostatni czytelnik opuszczajacy czytelnie)
        iret1 = sem_wait(&writter_acces);
        if(iret1){
            fprintf(stderr, "Błąd wywolania operacji wait na semaforze writter_acces. Kod błędu: %d\n", iret1);
            exit(EXIT_FAILURE);
        }

            show_queue_and_reading_room();  //  Wypisanie informacji o stanie kolejek i liczbie osób w czytelni
            writers_in_reading_room++;  // Pisarz wchodzi do czytelni - zwiększenie licznika pisarzy w czytelni
            dequeue(queue); // Pisarz wszedł do czytelni - usuwany jest z kolejki oczekujących
            num_writter_in_reading_room = *(writer_threadData->writer_num); // Numer pisarza przebywającego aktualnie w czytelni
            show_queue_and_reading_room();  //  Wypisanie informacji o stanie kolejek i liczbie osób w czytelni

            sleep(random_sleep_time()); //Pisarz pisze przez pewnien losowy czas

            writers_in_reading_room--;  // Pisarz wychodzi z czytelni - zmniejszenie licznika pisarzy w czytelni
            num_writter_in_reading_room = -1;   // -1 oznacza, że żaden pisarz aktualnie nie znajduje się w czytelni
        
        // Zwolnienie sekcji krytycznej - inny pisarz może wejść do czytelni
        iret1 = sem_post(&writter_acces);   
        if(iret1){
            fprintf(stderr, "Błąd wywolania operacji post na semaforze writter_acces. Kod błędu: %d\n", iret1);
            exit(EXIT_FAILURE);
        }

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

        // Zwolnienie ticketLocka - zwiększenie numeru ticketu i obudzenie wszystkich wątków do sprawdzenia swojego ticketu
        iret1 = unlock(&ticket_lock_writer);  
        if(iret1){
            fprintf(stderr, "Błąd odblokowania ticket_lock_writer. Kod błędu: %d\n", iret1);
            exit(EXIT_FAILURE);
        }

        sleep(random_sleep_time()); // Pisarz czeka pewnien losowy czas przez próbą kolejnego wejścia do czytelni (ustawienia się w kolejce)
        enqueue(queue, *(writer_threadData->writer_num));   // Pisarz dodawany jest do kolejki oczekujących na wejście
    }
}

// Funckja Initialize inicjalizuje wszytskie potrzebne do działania programu komponenty: mutexy, semafory, ticketLocki
// Sprawdza także czy każda z inicjalizacji przebiegła pomyślnie
// Jeśi nie to wypisywana jest informacja o błędzie na STDERR i program jest kończony
void Initialize(){
    int iret1;
    iret1 = init_ticket_lock2(&ticket_lock_writer);
    if(iret1){
        fprintf(stderr, "Błąd inicjacji ticket lock ticket_lock_writer. Kod błędu: %d\n", iret1);
        exit(EXIT_FAILURE);
    }
    
    iret1 = pthread_mutex_init(&reader_in_out, NULL);
    if(iret1){
        fprintf(stderr, "Błąd inicjacji mutexu reader_in_out. Kod błędu: %d\n", iret1);
        exit(EXIT_FAILURE);
    }   
    iret1 = sem_init(&writter_acces, 0, 1);
    if(iret1){
        fprintf(stderr, "Błąd inicjacji semafora writter_acces. Kod błędu: %d\n", iret1);
        exit(EXIT_FAILURE);
    }
}

// Funkcja Destroy niszczy wszytskie komponenty po zakończeniu działania programu: mutexy, semafory, ticketLocki
// Sprawdza także czy każde zniszczenie przebiegło pomyślnie
// Jeśi nie to wypisywana jest informacja o błędzie na STDERR i program jest kończony
void Destroy(){
    int iret1;
    iret1 = pthread_mutex_destroy(&reader_in_out);
    if (iret1){
        fprintf(stderr, "Błąd niszczenia mutexu reader_in_out. Kod błędu: %d\n", iret1);
        exit(EXIT_FAILURE);
    }
    iret1 = sem_destroy(&writter_acces);
    if (iret1){
        fprintf(stderr, "Błąd niszczenia semafora writter_acces. Kod błędu: %d\n", iret1);
        exit(EXIT_FAILURE);
    }
    iret1 = destroy_ticket_lock(&ticket_lock_writer);
    if(iret1){
        fprintf(stderr, "Błąd niszczenia ticket lock ticket_lock_writer. Kod błędu: %d\n", iret1);
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


