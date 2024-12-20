//
// Created by martin on 18/12/24.
//

#ifndef QUEUE_H
#define QUEUE_H

#include <pthread.h>
#include <stdbool.h>

#include "constants.h"

struct Node {
    char msg[MAX_MSG_SIZE];
    struct Node *next;
} typedef Node;

struct {
    Node *head;
    Node *tail;
    unsigned long size;
    pthread_mutex_t lock;
    pthread_cond_t cond;
} typedef Queue;

void enqueue(Queue *queue, char *msg);
Node *dequeue(Queue *queue);
char *peek_queue(Queue *queue);
Queue *init_queue(void);
void free_queue(Queue *queue);


#endif //QUEUE_H
