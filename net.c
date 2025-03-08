//
// Created by martin on 18/12/24.
//

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <time.h>
#include "net.h"
#include <systemd/sd-journal.h>

// get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa) {
    if (sa->sa_family == AF_INET) {
        return &(((struct sockaddr_in *) sa)->sin_addr);
    }

    return &(((struct sockaddr_in6 *) sa)->sin6_addr);
}

int connect_to_server(const char *hostname, const char *port, volatile int *alive) {
    struct addrinfo hints, *servinfo, *p;
    int rv;
    int sockfd;
    char s[INET6_ADDRSTRLEN];

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    sd_journal_print(LOG_INFO, "connecting to %s", hostname);

    while (*alive && (rv = getaddrinfo(hostname, port, &hints, &servinfo)) != 0) {
        sd_journal_print(LOG_ERR, "getaddrinfo: %s", gai_strerror(rv));
        sleep(5);
    }

    while (*alive) {
        // loop through results and connect to first we can
        for (p = servinfo; p != NULL; p = p->ai_next) {
            if ((sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1) {
                sd_journal_print(LOG_ERR, "socket creation failed");
                continue;
            }

            if (connect(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
                close(sockfd);
                sd_journal_print(LOG_ERR, "socket connection failed");
                continue;
            }

            break; // connected
        }

        // if not connected, retry
        if (p == NULL) {
            sd_journal_print(LOG_INFO, "retrying...");
            sleep(5);
            continue; // try again
        }

        inet_ntop(p->ai_family, get_in_addr((struct sockaddr *) p->ai_addr), s, sizeof s);

        freeaddrinfo(servinfo);

        // Successfully connected
        sd_journal_print(LOG_INFO, "connected to %s", s);
        return sockfd;
    }
    return -1;
}

int send_all(const int sockfd, const char *buf, size_t *len) {
    // sd_journal_print(LOG_INFO, "sending %u bytes to server", *len);
    size_t total = 0;
    size_t bytes_left = *len;
    int n = -1;

    while (total < *len) {
        if ((n = send(sockfd, buf + total, bytes_left, MSG_NOSIGNAL)) == -1) {
            break;
        }

        total += n;
        bytes_left -= n;
    }
    *len = total;

    // sd_journal_print(LOG_INFO, "sent %u bytes to server", total);
    // sd_journal_print(LOG_INFO, "returning %d", n == -1);

    return (n == -1) ? -1 : 0;
}
