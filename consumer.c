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

#define min(a,b) (((a) < (b)) ? (a) : (b))

void *consumer_thread(void *arg) {

    const ConsumerArgs *consumer_args = (ConsumerArgs *) arg;
    Queue *queue = consumer_args->queue;
    const char *hostname = consumer_args->hostname;
    const char *port = consumer_args->port;

    int sockfd = connect_to_server(hostname, port);

    printf("sockfd: %d\n", sockfd);

    if (sockfd == -1) {
        perror("Failed to connect to server");
        return NULL;
    }

    while (1) {
        char *msg = peek_queue(queue);
        if (strcmp(msg, "quit") == 0) {
            break;
        }

        const uint32_t msg_len = min(strlen(msg), MAX_MSG_SIZE - 1);
        const uint32_t network_msg_len = htonl(msg_len);
        size_t total_len = sizeof(network_msg_len) + msg_len;
        char send_buf[total_len];

        memcpy(send_buf, &network_msg_len, sizeof(network_msg_len));
        memcpy(send_buf + sizeof(network_msg_len), msg, msg_len);

        if (send_all(sockfd, send_buf, &total_len) == -1) {
            perror("send failed: message\n");
            close(sockfd);
            sockfd = connect_to_server(hostname, port); // try reconnecting on error
            continue;
        }

        printf("sent\t\t'%s'\n", msg);
        free(dequeue(queue));

    }

    close(sockfd);
    return NULL;
}
