//
// Created by martin on 18/12/24.
//

#include "consumer.h"
#include "net.h"
#include "queue.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <netinet/in.h>
#include <gpiod.h>

#define min(a,b) (((a) < (b)) ? (a) : (b))


void *consumer_thread(void *arg) {

    enum gpiod_line_value consumer_leds_off[] = {
        GPIOD_LINE_VALUE_INACTIVE,
        GPIOD_LINE_VALUE_INACTIVE
    };

    enum gpiod_line_value consumer_leds_green[] = {
        GPIOD_LINE_VALUE_ACTIVE,
        GPIOD_LINE_VALUE_INACTIVE
    };

    enum gpiod_line_value consumer_leds_red[] = {
        GPIOD_LINE_VALUE_INACTIVE,
        GPIOD_LINE_VALUE_ACTIVE
    };

    enum gpiod_line_value consumer_leds_on[] = {
        GPIOD_LINE_VALUE_ACTIVE,
        GPIOD_LINE_VALUE_ACTIVE
    };

    const ConsumerArgs *consumer_args = (ConsumerArgs *) arg;
    Queue *queue = consumer_args->queue;
    const char *hostname = consumer_args->hostname;
    const char *port = consumer_args->port;
    struct gpiod_line_request *led_request = consumer_args->led_request;
    volatile int *alive = consumer_args->alive;

    int sockfd = connect_to_server(hostname, port, alive);

    gpiod_line_request_set_values(led_request, consumer_leds_on);

    printf("sockfd: %d\n", sockfd);

    while (*alive) {
        // char *msg = peek_queue(queue, alive);
        char *msg;
        if ((msg = peek_queue(queue, alive)) == NULL) {
            break;
        }

        const uint32_t msg_len = min(strlen(msg), MAX_MSG_SIZE - 1);
        const uint32_t network_msg_len = htonl(msg_len);
        size_t total_len = sizeof(network_msg_len) + msg_len;
        char send_buf[total_len];

        memcpy(send_buf, &network_msg_len, sizeof(network_msg_len));
        memcpy(send_buf + sizeof(network_msg_len), msg, msg_len);

        if (send_all(sockfd, send_buf, &total_len) == -1) {
            gpiod_line_request_set_values(led_request, consumer_leds_red);
            perror("send failed: message\n");
            close(sockfd);
            sockfd = connect_to_server(hostname, port, alive); // try reconnecting on error
            continue;
        }

        gpiod_line_request_set_values(led_request, consumer_leds_green);
        // printf("sent: '%s'\n", msg);
        free(dequeue(queue));
    }

    gpiod_line_request_set_values(led_request, consumer_leds_off);
    close(sockfd);
    printf("Consumer thread exiting.\n");
    pthread_exit(NULL);
}
