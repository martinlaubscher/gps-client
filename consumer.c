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

    const ConsumerArgs *consumer_args = (ConsumerArgs *) arg;
    Queue *queue = consumer_args->queue;
    const char *hostname = consumer_args->hostname;
    const char *port = consumer_args->port;
    struct gpiod_line *red_led_line = consumer_args->red_led_line;
    struct gpiod_line *green_led_line = consumer_args->green_led_line;
    volatile int *alive = consumer_args->alive;

    int sockfd = connect_to_server(hostname, port, alive);

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
            gpiod_line_set_value(green_led_line, 0);
            gpiod_line_set_value(red_led_line, 1);
            perror("send failed: message\n");
            close(sockfd);
            sockfd = connect_to_server(hostname, port, alive); // try reconnecting on error
            continue;
        }

        gpiod_line_set_value(green_led_line, 1);
        gpiod_line_set_value(red_led_line, 0);
        printf("sent: '%s'\n", msg);
        free(dequeue(queue));

    }

    close(sockfd);
    printf("Consumer thread exiting.\n");
    pthread_exit(NULL);
}
