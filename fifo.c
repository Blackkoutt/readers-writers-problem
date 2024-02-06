#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include "fifo.h"

// Tworzenie nowej kolejki FIFO
FIFO* createQueue(){
    FIFO* queue = (FIFO*)malloc(sizeof(FIFO));  // Alokacja pamięci na kolejkę FIFO
    if(queue == NULL){  //Jeśli wystąpił błąd podczas alokacji wypisz komunikat i zakończ program
        fprintf(stderr, "Błąd alokacji pamięci dla kolejki FIFO");
        exit(EXIT_FAILURE);
    }
    // Ustawienie wskaźników na pierwszy i ostatni element kolejki jako NULL
    queue->front = NULL;
    queue->last = NULL;
}

// Sprawdza czy kolejka jest pusta 
bool isEmpty(FIFO *queue){

    // Kolejka jest pusta tylko wtedy kiedy nie posiada pierwszego elementu, jej pierwszy element jest równy NULL
    if(queue->front == NULL) return true;
    else return false;
}

// Dodanie nowego elementu do kolejki
void enqueue(FIFO* queue, int thread_number){
    Element* newElement = (Element*)malloc(sizeof(Element));    // Alokacja pamięci na nowy element kolejki
    if(newElement ==  NULL){    // Jeśli błąd alokacji wypisz komunikat i zakończ program
        fprintf(stderr, "Błąd alokacji pamięci dla nowego elementu kolejki FIFO");
        exit(EXIT_FAILURE);
    }

    // Dodanie danych (numeru wątku) do nowego elementu kolejki
    newElement->thread_num = thread_number;
    newElement->next = NULL;

    // Jeśli kolejka jest akutalnie pusta nowy element wstaw jako pierwszy i ostatni
    if(isEmpty(queue)){
        queue->front = newElement;
        queue->last = newElement;
    }

    // Jeśli kolejka nie jest pusta nowy element jest dodawany na koniec
    else{
        // Następny element od ostatniego wskazuje na nowy element 
        queue->last->next = newElement;
        // Ostatni element jest aktualizowany jako nowy element
        queue->last = newElement;
    }
}

// Zdjęcie pierwszego elementu z kolejki
void dequeue(FIFO *queue){

    // Jeśli kolejka jest pusta funkcja wraca z wywołania (nie ma co zdjemować)
    if(isEmpty(queue)){
        return;
    }

    Element* temp = queue->front; // Przypisanie pierwszego elementu kolejki do zmiennej pomocniczej
    int data = temp->thread_num;    // Przypisanie zawartości pierwszego elementu do zmiennej pomocniczej
    queue->front=queue->front->next;    // Przesunięcie wskaźnika na pierwszy element kolejki na jej drugi element

    // Jeśli po przesunięciu początek kolejki jest równy NULL (kolejka pusta)
    // Ustawienie ostatniego elemetu także na wartość NULL
    if(queue->front == NULL){
        queue->last = NULL;
    }

    free(temp); // Zwolnienie pamięci zdjętego z kolejki elementu
}

// Wyświetlenie zawartości kolejki
void displayQueue(FIFO* queue){

    // Jeśli kolejka pusta wyświetlany jest stosowny komunikat
    if(isEmpty(queue)){
        printf("Kolejka pusta\n");
        return;
    }

    // W przeciwnym wypadku należy przejść po wszytskich elementach kolejki i wypisać ich zawartość
    Element* current = queue->front;
    while(current != NULL){
        printf("%d ", current->thread_num);
        current=current->next;
    }
    printf("\n");
}

// Zwraca ilość elementów w kolejce
int queueCapacity(FIFO* queue){

    // Jeśli kolejka jest pusta zwróć 0
    if(isEmpty(queue)){
        return 0;
    }
    
    // Jeśli kolejka nie jest pusta przejdź po wszytskich elementach zliczając je
    Element* current = queue->front;
    int capacity = 0;
    while(current != NULL){
        capacity+=1;
        current=current->next;
    }
    return capacity;    // Zwróć ilość elementów kolejki
}

