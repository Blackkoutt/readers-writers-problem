#ifndef FIFO_H
#define FIFO_H
#include "list.h"

typedef struct Element{
    int thread_num;
    struct Element* next;
}Element;

typedef struct FIFO{
    Element* front;
    Element* last;
} FIFO;

FIFO* createQueue();
bool isEmpty(FIFO *queue);
void enqueue(FIFO* queue, int thread_number);
void dequeue(FIFO *queue);
void displayQueue(FIFO* queue);
int queueCapacity(FIFO* queue);

#endif