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

#include "net.h"


// get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa) {
    if (sa->sa_family == AF_INET) {
        return &(((struct sockaddr_in *) sa)->sin_addr);
    }

    return &(((struct sockaddr_in6 *) sa)->sin6_addr);
}

int connect_to_server(const char *hostname, const char *port) {
    struct addrinfo hints, *servinfo, *p;
    int rv;
    int sockfd;
    char s[INET6_ADDRSTRLEN];

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    printf("Trying to connect to %s\n", hostname);

    while ((rv = getaddrinfo(hostname, port, &hints, &servinfo)) != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
        sleep(5);
    }

    while (1) {
        // loop through results and connect to first we can
        for (p = servinfo; p != NULL; p = p->ai_next) {
            if ((sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1) {
                perror("socket creation failed");
                continue;
            }

            if (connect(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
                close(sockfd);
                perror("socket connection failed");
                continue;
            }

            break; // connected
        }

        // if not connected, retry
        if (p == NULL) {
            fprintf(stderr, "retrying...\n\n");
            sleep(5);
            continue; // try again
        }

        inet_ntop(p->ai_family, get_in_addr((struct sockaddr *) p->ai_addr), s, sizeof s);
        printf("connecting to %s\n", s);

        freeaddrinfo(servinfo);

        // Successfully connected
        printf("connected\n");
        return sockfd;
    }
}

int send_all(const int sockfd, const char *buf, size_t *len) {
    size_t total = 0;
    size_t bytes_left = *len;
    int n = -1;

    while (total < *len) {
        if ((n = send(sockfd, buf + total, bytes_left,  MSG_NOSIGNAL)) == -1) {
            break;
        }

        total += n;
        bytes_left -= n;
    }
    *len = total;

    return (n == -1) ? -1 : 0;
}
