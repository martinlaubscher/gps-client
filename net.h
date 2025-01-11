//
// Created by martin on 18/12/24.
//

#ifndef NET_H
#define NET_H

#include <stddef.h>
#include <sys/types.h>
#include <stdint.h>

int connect_to_server(const char *hostname, const char *port, volatile int *alive);
int send_all(int sockfd, const char *buf, size_t *len);

#endif //NET_H
