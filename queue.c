//
// Created by martin on 18/12/24.
//

#include <stdlib.h>
#include <string.h>

#include "constants.h"
#include "queue.h"

#include <stdio.h>

/* "private" function prototypes */
void discard(Queue *);

bool is_full(Queue *);

bool is_empty(Queue *);

unsigned long size(Queue *);
/* "private" function prototypes end */


void enqueue(Queue *queue, char *msg) {
    Node *newNode = malloc(sizeof(Node));
    memset(newNode, 0, sizeof(Node));
    strncpy(newNode->msg, msg, MAX_MSG_SIZE);
    newNode->next = NULL;

    pthread_mutex_lock(&queue->lock);

    if (is_full(queue)) {
        discard(queue);
    }

    if (is_empty(queue)) {
        queue->head = newNode;
    } else {
        queue->tail->next = newNode;
    }

    queue->tail = newNode;
    ++(queue->size);

    pthread_cond_signal(&queue->cond);
    pthread_mutex_unlock(&queue->lock);
}


Node *dequeue(Queue *queue) {
    pthread_mutex_lock(&queue->lock);

    Node *tmp = queue->head;
    queue->head = tmp->next;
    tmp->next = NULL;
    --(queue->size);

    pthread_mutex_unlock(&queue->lock);

    return tmp;
}


char *peek_queue(Queue *queue) {
    pthread_mutex_lock(&queue->lock);
    while (queue->head == NULL) {
        pthread_cond_wait(&queue->cond, &queue->lock);
    }

    char *head_msg = queue->head->msg;

    pthread_mutex_unlock(&queue->lock);

    return head_msg;
}

Queue *init_queue(void) {
    Queue *queue = malloc(sizeof(Queue));
    queue->head = NULL;
    queue->tail = NULL;
    queue->size = 0;
    pthread_mutex_init(&queue->lock, NULL);
    pthread_cond_init(&queue->cond, NULL);
    return queue;
}


void free_queue(Queue *queue) {
    while (!is_empty(queue)) {
        Node *next = queue->head->next;
        free(queue->head);
        queue->head = next;
    }
    queue->tail = NULL;
    queue->size = 0;

    free(queue);
}


void discard(Queue *queue) {
    Node *tmp = queue->head;
    queue->head = tmp->next;
    free(tmp);
    --(queue->size);
}


bool is_full(Queue *queue) {
    return queue->size == MAX_QUEUE_SIZE;
}


bool is_empty(Queue *queue) {
    return queue->head == NULL;
}


unsigned long size(Queue *queue) {
    pthread_mutex_lock(&queue->lock);
    unsigned long size = queue->size;
    pthread_mutex_unlock(&queue->lock);
    return size;
}
