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

    char recv_buf[MAX_MSG_SIZE];
    char send_buf[LEN_PREFIX_SIZE + MAX_MSG_SIZE];

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

        const u_int32_t msg_len = min(strlen(msg), MAX_MSG_SIZE - 1);
        const uint32_t network_msg_len = htonl(msg_len);
        size_t total_len = sizeof(network_msg_len) + msg_len;

        memcpy(send_buf, &network_msg_len, sizeof(network_msg_len));
        memcpy(send_buf + sizeof(network_msg_len), msg, msg_len);
        // send_buf[total_len] = '\0';

        if (send_all(sockfd, send_buf, &total_len) == -1) {
            perror("send failed: message\n");
            close(sockfd);
            sockfd = connect_to_server(hostname, port); // try reconnecting on error
            continue;
        }

        printf("sent\t\t'%s'\n", msg);
        free(dequeue(queue));

        long numbytes = recv(sockfd, recv_buf, MAX_MSG_SIZE - 1, MSG_NOSIGNAL);
        if (numbytes == -1) {
            perror("recv failed");
            close(sockfd);
            sockfd = connect_to_server(hostname, port); // try reconnecting on error
            continue;
        }

        recv_buf[numbytes] = '\0';
        printf("received\t'%s'\n\n", recv_buf);
        // sleep(1);

    }

    close(sockfd);
    return NULL;
}
