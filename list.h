#ifndef LIST_H
#define LIST_H

typedef struct Node{
    int num;
    struct Node* next;
}Node;

void insert_element(Node **head, int value);
void remove_element(Node **head, int value);
void print_list(Node* head);
int list_count(Node* head);
void clear_list(Node **head);
bool element_exists(Node* head, int value);

#endif