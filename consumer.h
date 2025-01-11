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
    struct gpiod_line *green_led_line;
    struct gpiod_line *red_led_line;
    volatile int *alive;
} ConsumerArgs;

#endif //CONSUMER_H
