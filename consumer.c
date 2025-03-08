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
#include <netinet/tcp.h>
#include <gpiod.h>
#include <time.h>
#include <systemd/sd-journal.h>

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

    int tcp_timeout = 400; // milliseconds
    if (setsockopt(sockfd, IPPROTO_TCP, TCP_USER_TIMEOUT, &tcp_timeout, sizeof(tcp_timeout)) < 0) {
        sd_journal_print(LOG_ERR, "failed to set TCP_USER_TIMEOUT");
    }

    gpiod_line_request_set_values(led_request, consumer_leds_on);

    sd_journal_print(LOG_INFO, "sockfd: %d", sockfd);

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

        // sd_journal_print(LOG_INFO, "sending message: %s", msg);

        if (send_all(sockfd, send_buf, &total_len) == -1) {
            sd_journal_print(LOG_ERR, "send failed: %s", msg);
            if (gpiod_line_request_set_values(led_request, consumer_leds_red) == -1) {
                sd_journal_print(LOG_ERR, "could not set LED red");
            };
            close(sockfd);
            if (*alive) {
                sd_journal_print(LOG_ERR, "alive and about to reconnect");
                sockfd = connect_to_server(hostname, port, alive); // try reconnecting on error
            }
            continue;
        }

        gpiod_line_request_set_values(led_request, consumer_leds_green);
        // printf("sent: '%s'\n", msg);
        free(dequeue(queue));
    }
    gpiod_line_request_set_values(led_request, consumer_leds_off);
    close(sockfd);
    sd_journal_print(LOG_INFO, "Consumer thread exiting.");
    pthread_exit(NULL);
}
