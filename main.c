/*
** client.c -- a stream socket client demo
*/

#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "consumer.h"
#include "net.h"
#include "producer.h"
#include "queue.h"


int main(int argc, char *argv[]) {

    if (argc != 3) {
        fprintf(stderr, "usage: client hostname port\n");
        exit(1);
    }

    Queue *queue = init_queue();

    ConsumerArgs consumer_args;
    consumer_args.hostname = argv[1];
    consumer_args.port = argv[2];
    consumer_args.queue = queue;

    pthread_t prod_tid, cons_tid;
    pthread_create(&prod_tid, NULL, producer_thread, queue);
    pthread_create(&cons_tid, NULL, consumer_thread, &consumer_args);

    pthread_join(prod_tid, NULL);
    pthread_join(cons_tid, NULL);

    printf("exiting\n");
    pthread_mutex_destroy(&queue->lock);
    pthread_cond_destroy(&queue->cond);
    free_queue(queue);
    queue = NULL;

    return 0;
}
