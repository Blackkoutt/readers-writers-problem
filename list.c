#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include "list.h"

// Dodanie nowego elementu do listy 
void insert_element(Node **head, int value){
    Node *new_node = (Node*) malloc(sizeof(Node));  // Alokacja pamięci na nowy element listy
    if (new_node == NULL) { // Sprawdzenie poprawności alokacji
        fprintf(stderr, "Błąd dodawania nowego elementu listy.");
        exit(EXIT_FAILURE);
    }

    // Dodanie danych nowemu elementowi
    new_node->num = value;
    new_node->next = NULL;

    // Jeśli lista jest pusta wstaw nowy element jako pierwszy
    if (*head == NULL) {
        *head = new_node;
    }

    // W przeciwnym wypadku należy znaleźć ostatni element listy i nowy element wstawić jako jego następnik
    else { 
        Node *current = *head;  // Przypisanie do zmiennej pomocniczej
        while (current->next != NULL) { // Przeszukanie listy do konca
            current = current->next;
        }
        current->next = new_node; // Dodanie elementu na koncu listy
    }
}

// Usunięcie określonego elementu z listy
void remove_element(Node **head, int value){
    Node *current = *head; // Początek listy
    Node *previous = NULL;  // Poprzedni element

    // Przeszukanie listy do końca
    while(current != NULL){
        // Jeśli aktualny element zawiera dane do usunięcia
        if(current->num == value){
            // Jeśli poprzedni element jest równy NULL, to do usunięcia jest pierwszy element listy
            if(previous == NULL){
                // Przesuń wskaźnik początku listy na drugi element
                *head = current->next;
            }
            // W przeciwnym wypadku do usunięcia jest element ze środka listy
            else{
                // Przesunięcie wskaźnika następnego elementu od poprzedniego elementu na następny element od aktualnego
                // W ten sposób pomijany jest aktualny element 
                previous->next=current->next;
            }
            free(current);  // Zwolnij pamięć dla aktualnego elementu
            return;
        }  
        previous = current; // Poprzedni element jest teraz aktualnym elementem
        current = current->next;    // Aktualny jest następnym elementem akutalnego
    } 
}

//Sprawdza czy podany element występuje na liście podanej jako parametr
bool element_exists(Node* head, int value) {

    Node* current = head;

    // Jeśli pierwszy element listy jest równy NULL to znaczy, że lista jest pusta
    if(current==NULL){
        return false;   // Zwróć false - brak elementu
    }

    // W przeciwnym wypadku przeszukaj całą listę
    while (current != NULL) {
        if(current->num==value){
            return true;    // Jeśli znaleziono zwróć true
        }
        current = current->next;
    }
    return false;   // Jeśli przeszukano całą listę i nie znaleziono pasującego elementu zwróć false
}

// Usuwa wszystkie elementy listy
void clear_list(Node **head){
    Node* current = *head;
    Node* next;

    //Przejdz po całej liście od jej początku
    while (current != NULL) {
        next = current->next;   // Przypisz następny element listy do zmiennej pomocniczej
        free(current);  // Zwolnij pamięć aktualnego elementu
        current = next; // Przejdź do kolejnego elementu listy
    }

    *head = NULL;   // Ustaw wskaźnik na pierwszy element listy jako wartość NULL
}

// Zwraca pojemność listy (ilość elementów)
int list_count(Node* head){
    int count = 0;
    Node* current = head;

    // Przejdź po całej liście zwiększając licznik elementów
    while (current != NULL) {
        count++;
        current = current->next;
    }

    return count;   // Zwróć ilość elementów listy
}

// Wypisanie zawartości listy
void print_list(Node* head) {
    Node* current = head;

    // Jeśli pierwszy element listy jest równy NULL to znaczy, że lista jest pusta
    if(current==NULL){
        printf("Brak\n");
        return;
    }
    
    // Wypisuj dane każdego elementu z listy
    while (current != NULL) {
        printf("%d ", current->num);
        current = current->next;
    }
    printf("\n");
}