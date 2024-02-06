#include <pthread.h>
#include "ticket_lock.h"

// Inizjalizacja mutexu i zmiennej warunkowej potrzebnej do zrealizowania idei TicketLock i sprawdzenie jej poprawności 
int init_ticket_lock (TicketLock *ticket_lock){
    // W tym przypadku ticket ustawiany jest na 1 a aktualny ticket na 0 co prowadzi do zablokowania pierwszego wątku wchodzącego
    // do TicketLock'a
    ticket_lock->current_ticket = 0;
    ticket_lock->ticket = 1;
    int iret1;
    iret1 = pthread_mutex_init(&(ticket_lock->mutex), NULL);
    if(iret1){
        return iret1;
    }
    iret1 = pthread_cond_init(&(ticket_lock->cond), NULL);
    if(iret1){
        return iret1;
    }
    return 0;
}
int init_ticket_lock2 (TicketLock *ticket_lock){
    // Ticket i aktualny ticket są ustawiane na 0, co oznacza, że pierwszy wątek który wejdzie do funckji lock
    // nie będzie oczekiwać na swój numer, co zapobiega blokadzie
    ticket_lock->current_ticket = 0;
    ticket_lock->ticket = 0;
    int iret1;
    iret1 = pthread_mutex_init(&(ticket_lock->mutex), NULL);
    if(iret1){
        return iret1;
    }
    iret1 = pthread_cond_init(&(ticket_lock->cond), NULL);
    if(iret1){
        return iret1;
    }
    return 0;
}

// Usunięcie i sprawdzenie poprawności usunięcia mutexu i zmiennej warunkowej potrzebnej w TicketLock
int destroy_ticket_lock(TicketLock *ticket_lock){
    int iret1;
    iret1 = pthread_mutex_destroy(&(ticket_lock->mutex));
    if(iret1){
        return iret1;
    }
    iret1 = pthread_cond_destroy(&(ticket_lock->cond));
    if(iret1){
        return iret1;
    }
    return 0;
}

// Służy do zakolejkowania kolejnych wątków
// Wątki oczekują w tej funckji tak długo aż nie zostanie podany przypisany im ticket
int lock(TicketLock *ticket_lock){
    int iret1;
    int thread_ticket;  // ticket aktualnego wątku

    // Blokada mutexu (sekcja krytyczna) - dostęp do współdzielonego zasobu: ticket
    iret1 = pthread_mutex_lock(&(ticket_lock->mutex));
    if(iret1){
        return iret1;
    }
    thread_ticket=ticket_lock->ticket;  // Wątkowi przypisywany jest numer ticketu
    ticket_lock->ticket++;  // Numer ticketu jest zwiększany o 1

    // Dopóki aktualny ticket nie jest równy ticketowi wątku
    // Wątek czeka na zmiennej warunkowej ticket_lock->cond na swój ticket
    while(ticket_lock->current_ticket != thread_ticket){
        iret1 = pthread_cond_wait(&(ticket_lock->cond), &(ticket_lock->mutex));
        if(iret1){
            return iret1;
        }

    }
    // Jeśli aktualny ticket był równy ticketowi wątku to wychodzi on z kolejki
    // Takie podejście umożliwia prostszy sposób zarządania kolejnością wykonania wątków oraz dołączanie ich numerów do kolejki FIFO
    // Bez obawy o zdjęcie z kolejki wątku który aktualnie się nie wykonuje
    
    iret1 = pthread_mutex_unlock(&(ticket_lock->mutex));    // Zwolnienie muteksu (zwolenienie sekcji krytycznej)
    if(iret1){
        return iret1;
    }
    return 0;
}

// Kiedy wątek zwalnia mutex przechodzi do wykonania funkcji unlock za pomocą której zwiększa numer aktualnego ticketu
// Budzi też wszytskie wątki oczekujące na zmiennej warunkowej ticket_lock->cond w funkcji lock tym samym każdy oczekujący wątek
// sprawdza czy został podany jego ticket
int unlock(TicketLock *ticket_lock){
    int iret1;
    iret1 = pthread_mutex_lock(&(ticket_lock->mutex));  // Zablokowanie mutexu (sekcja krytyczna) - modyfikacja zasobu current_ticket
    if(iret1){
        return iret1;
    }

    // Obudzenie wszytskich wątków oczekujących na zmiennej warunkowej ticket_lock->cond w funkcji lock 
    // Każdy oczekujący wątek porównuje swój ticket z aktualnym ticketem
    iret1 = pthread_cond_broadcast(&(ticket_lock->cond)); 
    if(iret1){
        return iret1;
    }
    ticket_lock->current_ticket+=1;  // Zwiększenie aktualnego ticketu o 1
    

    iret1 = pthread_mutex_unlock(&(ticket_lock->mutex));    // Zwolnienie mutexu (wyjście z sekcji krytycznej)
    if(iret1){
        return iret1;
    }
    return 0;
}