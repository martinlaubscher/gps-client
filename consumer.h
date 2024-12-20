//
// Created by martin on 18/12/24.
//

#ifndef CONSUMER_H
#define CONSUMER_H

#include "queue.h"

void *consumer_thread(void *arg);

struct {
    Queue *queue;
    char *hostname;
    char *port;
} typedef ConsumerArgs;

#endif //CONSUMER_H
