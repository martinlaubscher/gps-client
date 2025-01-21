//
// Created by martin on 18/12/24.
//

#ifndef PRODUCER_H
#define PRODUCER_H

#include "queue.h"

void *producer_thread(void *arg);

typedef struct ProducerArgs {
    Queue *queue;
    struct gpiod_line_request *led_request;
    volatile int *alive;
} ProducerArgs;

#endif //PRODUCER_H
