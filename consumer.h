//
// Created by martin on 18/12/24.
//

#ifndef CONSUMER_H
#define CONSUMER_H

#include "queue.h"

void *consumer_thread(void *arg);

typedef struct ConsumerArgs {
    Queue *queue;
    char *hostname;
    char *port;
    struct gpiod_line_request *led_request;
    volatile int *alive;
} ConsumerArgs;

#endif //CONSUMER_H
