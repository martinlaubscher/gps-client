//
// Created by martin on 18/12/24.
//

#include "producer.h"
#include "queue.h"

#include <stdio.h>
#include <unistd.h>

void *producer_thread(void *arg) {

    Queue *queue = (Queue*) arg;

    for (int i = 0; i < 10; ++i) {
        char msg[MAX_MSG_SIZE] = {0};
        sprintf(msg, "+CGNSSINFO: %d,05,02,03,473%d.235748,N,00738.929515,E,161224,144959.0,317.2,0.0,,2.1,1.9,0.8", i, i);
        // printf("message: %s\n", msg);

        enqueue(queue, msg);

        sleep(1);
    }

    enqueue(queue, "quit");

    return NULL;
}
