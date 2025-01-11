//
// Created by martin on 18/12/24.
//

#ifndef PRODUCER_H
#define PRODUCER_H

#include "queue.h"

void *producer_thread(void *arg);

typedef struct GpsLedLines {
    struct gpiod_line *red;
    struct gpiod_line *yellow;
    struct gpiod_line *green;
} GpsLedLines;

typedef struct ProducerArgs {
    Queue *queue;
    GpsLedLines *gps_led_lines;
    volatile int *alive;
} ProducerArgs;

#endif //PRODUCER_H
