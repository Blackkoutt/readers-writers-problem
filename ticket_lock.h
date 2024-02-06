#ifndef TICKET_LOCK_H
#define TICKET_LOCK_H

typedef struct{
    unsigned long int current_ticket;
    unsigned long int ticket;
    pthread_mutex_t mutex;
    pthread_cond_t cond;
} TicketLock;

int init_ticket_lock (TicketLock *ticket_lock);
int init_ticket_lock2 (TicketLock *ticket_lock);
int destroy_ticket_lock(TicketLock *ticket_lock);
int lock(TicketLock *ticket_lock);
int unlock(TicketLock *ticket_lock);

#endif
